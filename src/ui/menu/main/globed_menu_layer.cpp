#include "globed_menu_layer.hpp"

#include <UIBuilder.hpp>

#include "server_list_cell.hpp"
#include <ui/menu/player_list/player_list_popup.hpp>
#include <ui/menu/server_switcher/server_switcher_popup.hpp>
#include <util/ui.hpp>
#include <util/net.hpp>
#include <net/network_manager.hpp>
#include <managers/error_queues.hpp>
#include <managers/account_manager.hpp>

using namespace geode::prelude;

bool GlobedMenuLayer::init() {
    if (!CCLayer::init()) return false;

    GlobedAccountManager::get().autoInitialize();

    auto winsize = CCDirector::get()->getWinSize();

    // server list

    auto listview = Build<ListView>::create(createServerList(), ServerListCell::CELL_HEIGHT, LIST_WIDTH, LIST_HEIGHT)
        .collect();

    Build<GJListLayer>::create(listview, "Servers", ccc4(0, 0, 0, 180), LIST_WIDTH, 220.f)
        .zOrder(2)
        .anchorPoint(0.f, 0.f)
        .parent(this)
        .id("server-list"_spr)
        .store(listLayer);

    listLayer->setPosition({winsize / 2 - listLayer->getScaledContentSize() / 2});

    Build<GlobedSignupLayer>::create()
        .zOrder(2)
        .anchorPoint(0.f, 0.f)
        .pos(listLayer->getPosition())
        .parent(this)
        .id("signup-layer"_spr)
        .store(signupLayer);

    // left button menu

    auto* leftButtonMenu = Build<CCMenu>::create()
        .layout(
            ColumnLayout::create()
                ->setAutoScale(true)
                ->setGap(5.0f)
                ->setAxisAlignment(AxisAlignment::Start)
        )
        .anchorPoint(0.f, 0.f)
        .pos(15.f, 15.f)
        .parent(this)
        .id("left-button-menu"_spr)
        .collect();

    // refresh servers btn

    Build<CCSprite>::createSpriteName("miniSkull_001.png")
        .scale(1.2f)
        .intoMenuItem([](auto) {
            // this->requestServerList();
            if (auto* popup = PlayerListPopup::create()) {
                popup->m_noElasticity = true;
                popup->show();
            }
        })
        .id("btn-refresh-servers"_spr)
        .parent(leftButtonMenu);

    // TODO prod remove wipe authtoken button

    Build<CCSprite>::createSpriteName("d_skull01_001.png")
        .scale(1.2f)
        .intoMenuItem([](auto) {
            GlobedAccountManager::get().clearAuthKey();
        })
        .id("btn-clear-authtoken"_spr)
        .parent(leftButtonMenu);

    // server switcher button

    Build<CCSprite>::createSpriteName("gjHand_05_001.png")
        .scale(1.2f)
        .intoMenuItem([](auto) {
            if (auto* popup = ServerSwitcherPopup::create()) {
                popup->m_noElasticity = true;
                popup->show();
            }
        })
        .id("btn-open-server-switcher"_spr)
        .parent(leftButtonMenu);

    leftButtonMenu->updateLayout();

    // TODO: menu for connecting to a standalone server directly with an IP and port
    // it must call the proper func in GlobedServerManager::addGameServer then try to NM::connectStandalone

    util::ui::addBackground(this);

    auto menu = CCMenu::create();
    this->addChild(menu);

    util::ui::addBackButton(menu, util::ui::navigateBack);

    this->setKeyboardEnabled(true);
    this->setKeypadEnabled(true);

    CCScheduler::get()->scheduleSelector(schedule_selector(GlobedMenuLayer::refreshServerList), this, 0.1f, false);
    CCScheduler::get()->scheduleSelector(schedule_selector(GlobedMenuLayer::pingServers), this, 5.0f, false);

    auto& gsm = GameServerManager::get();

    if (gsm.count() == 0) {
        this->requestServerList();
    }

    gsm.pendingChanges = true; // force ping all servers
    this->refreshServerList(0.f);

    return true;
}

GlobedMenuLayer::~GlobedMenuLayer() {
    this->cancelWebRequest();
}

CCArray* GlobedMenuLayer::createServerList() {
    auto ret = CCArray::create();

    auto& nm = NetworkManager::get();
    auto& gsm = GameServerManager::get();

    bool authenticated = nm.established();

    auto activeServer = gsm.active();

    for (const auto& [serverId, server] : gsm.getAllServers()) {
        bool active = authenticated && serverId == activeServer;
        auto cell = ServerListCell::create(server, active);
        ret->addObject(cell);
    }

    return ret;
}

void GlobedMenuLayer::refreshServerList(float) {
    auto& am = GlobedAccountManager::get();
    auto& csm = CentralServerManager::get();

    // if we do not have a session token from the central server, and are not in a standalone server, don't show game servers
    if (!csm.standalone() && !am.hasAuthKey()) {
        listLayer->setVisible(false);
        signupLayer->setVisible(true);
        return;
    }

    signupLayer->setVisible(false);
    listLayer->setVisible(true);

    // if we recently switched a central server, redo everything
    if (csm.recentlySwitched) {
        csm.recentlySwitched = false;
        this->cancelWebRequest();
        this->requestServerList();
    }

    // if there are pending changes, hard refresh the list and ping all servers
    auto& gsm = GameServerManager::get();
    if (gsm.pendingChanges) {
        gsm.pendingChanges = false;

        listLayer->m_listView->removeFromParent();

        auto serverList = createServerList();
        listLayer->m_listView = Build<ListView>::create(serverList, ServerListCell::CELL_HEIGHT, LIST_WIDTH, LIST_HEIGHT)
            .parent(listLayer)
            .collect();

        this->pingServers(0.f);

        return;
    }

    // if there were no pending changes, still update the server data (ping, players, etc.)

    auto listCells = listLayer->m_listView->m_tableView->m_contentLayer->getChildren();
    if (listCells == nullptr) {
        return;
    }

    auto active = gsm.active();

    bool authenticated = NetworkManager::get().established();

    for (auto* obj : CCArrayExt<CCNode>(listCells)) {
        auto slc = static_cast<ServerListCell*>(obj->getChildren()->objectAtIndex(2));
        auto server = gsm.getServer(slc->gsview.id);
        slc->updateWith(server, authenticated && slc->gsview.id == active);
    }
}

void GlobedMenuLayer::requestServerList() {
    this->cancelWebRequest();

    auto& csm = CentralServerManager::get();

    if (csm.standalone()) {
        return;
    }

    NetworkManager::get().disconnect(false);

    auto centralUrl = csm.getActive();

    if (!centralUrl) {
        return;
    }

    serverRequestHandle = GHTTPRequest::get(fmt::format("{}/servers", centralUrl.value().url))
        .userAgent(util::net::webUserAgent())
        .timeout(util::time::secs(3))
        .then([this](const GHTTPResponse& response) {
            this->serverRequestHandle = std::nullopt;
            if (response.anyfail()) {
                ErrorQueues::get().error(fmt::format("Failed to fetch servers: <cy>{}</c>", response.anyfailmsg()));

                auto& gsm = GameServerManager::get();
                gsm.clear();
                gsm.pendingChanges = true;
            } else {
                auto jsonResponse = json::Value::from_str(response.response);

                auto& gsm = GameServerManager::get();
                gsm.clear();
                gsm.pendingChanges = true;

                try {
                    auto serverList = jsonResponse.as_array();
                    for (const auto& obj : serverList) {
                        auto server = obj.as_object();
                        gsm.addServer(
                            server["id"].as_string(),
                            server["name"].as_string(),
                            server["address"].as_string(),
                            server["region"].as_string()
                        );
                    }
                } CATCH {
                    ErrorQueues::get().error("Failed to parse server list: <cy>{}</c>", CATCH_GET_EXC);
                    gsm.clear();
                }
            }
        })
        .send();

    geode::log::debug("rqeuested server list");
}

void GlobedMenuLayer::cancelWebRequest() {
    if (serverRequestHandle.has_value()) {
        serverRequestHandle->cancel();
        serverRequestHandle = std::nullopt;
        return;
    }
}

void GlobedMenuLayer::keyBackClicked() {
    util::ui::navigateBack();
}

void GlobedMenuLayer::pingServers(float) {
    NetworkManager::get().taskPingServers();
}

GlobedMenuLayer* GlobedMenuLayer::create() {
    auto ret = new GlobedMenuLayer;
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }

    CC_SAFE_DELETE(ret);
    return nullptr;
}

cocos2d::CCScene* GlobedMenuLayer::scene() {
    auto layer = GlobedMenuLayer::create();
    auto scene = cocos2d::CCScene::create();
    layer->setPosition({0.f, 0.f});
    scene->addChild(layer);

    return scene;
}