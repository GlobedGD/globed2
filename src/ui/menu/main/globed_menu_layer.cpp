#include "globed_menu_layer.hpp"

#include "server_list_cell.hpp"
#include <ui/menu/room/room_popup.hpp>
#include <ui/menu/server_switcher/server_switcher_popup.hpp>
#include <ui/menu/settings/settings_layer.hpp>
#include <util/ui.hpp>
#include <util/net.hpp>
#include <net/network_manager.hpp>
#include <managers/error_queues.hpp>
#include <managers/account.hpp>
#include <data/types/misc.hpp>

using namespace geode::prelude;

bool GlobedMenuLayer::init() {
    if (!CCLayer::init()) return false;

    GlobedAccountManager::get().autoInitialize();

    auto winsize = CCDirector::get()->getWinSize();

    // server list

    auto listview = Build<ListView>::create(createServerList(), ServerListCell::CELL_HEIGHT, LIST_WIDTH, LIST_HEIGHT)
        .collect();

    Build<GJListLayer>::create(listview, "Servers", ccc4(0, 0, 0, 180), LIST_WIDTH, 220.f, 0)
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
            if (auto* popup = RoomPopup::create()) {
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

    // settings button

    Build<CCSprite>::createSpriteName("GJ_optionsBtn_001.png")
        .scale(1.0f)
        .intoMenuItem([](auto) {
            GlobedSettingsLayer::scene();
        })
        .id("btn-open-settings"_spr)
        .parent(leftButtonMenu);

    leftButtonMenu->updateLayout();

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

void GlobedMenuLayer::onExit() {
    CCLayer::onExit();
    this->cancelWebRequest();
}

CCArray* GlobedMenuLayer::createServerList() {
    auto ret = CCArray::create();

    auto& nm = NetworkManager::get();
    auto& gsm = GameServerManager::get();

    bool authenticated = nm.established();

    auto activeServer = gsm.getActiveId();

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

    auto active = gsm.getActiveId();

    bool authenticated = NetworkManager::get().established();

    for (auto* obj : CCArrayExt<CCNode*>(listCells)) {
        auto slc = static_cast<ServerListCell*>(obj->getChildren()->objectAtIndex(2));
        auto server = gsm.getServer(slc->gsview.id);
        if (server.has_value()) {
            slc->updateWith(server.value(), authenticated && slc->gsview.id == active);
        }
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

    serverRequestHandle = web::AsyncWebRequest()
        .userAgent(util::net::webUserAgent())
        .timeout(util::time::secs(3))
        .get(fmt::format("{}/servers", centralUrl.value().url))
        .text()
        .then([this](std::string& response) {
            this->serverRequestHandle = std::nullopt;
            auto& gsm = GameServerManager::get();
            gsm.clear();
            gsm.pendingChanges = true;

            auto decoded = util::crypto::base64Decode(response);
            if (decoded.size() < sizeof(NetworkManager::SERVER_MAGIC)) {
                ErrorQueues::get().error("Failed to parse server list: invalid response sent by the server (no magic)");
                gsm.clear();
                return;
            }

            ByteBuffer buf(std::move(decoded));
            auto magic = buf.readBytes(10);

            // compare it with the needed magic
            bool correct = true;
            for (size_t i = 0; i < sizeof(NetworkManager::SERVER_MAGIC); i++) {
                if (magic[i] != NetworkManager::SERVER_MAGIC[i]) {
                    correct = false;
                    break;
                }
            }

            if (!correct) {
                ErrorQueues::get().error("Failed to parse server list: invalid response sent by the server (invalid magic)");
                gsm.clear();
                return;
            }

            auto serverList = buf.readValueVector<GameServerEntry>();

            for (const auto& server : serverList) {
                auto result = gsm.addServer(
                    server.id,
                    server.name,
                    server.address,
                    server.region
                );

                if (result.isErr()) {
                    geode::log::warn("invalid server found when parsing response: {}", server.id);
                }
            }
        })
        .expect([this](std::string error) {
            this->serverRequestHandle = std::nullopt;
            ErrorQueues::get().error(fmt::format("Failed to fetch servers: <cy>{}</c>", error));

            auto& gsm = GameServerManager::get();
            gsm.clear();
            gsm.pendingChanges = true;
        })
        .send();

    geode::log::debug("rqeuested server list");
}

void GlobedMenuLayer::cancelWebRequest() {
    if (serverRequestHandle.has_value()) {
        serverRequestHandle->get()->cancel();
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

cocos2d::CCScene* GlobedMenuLayer::scene() {
    auto layer = GlobedMenuLayer::create();
    auto scene = cocos2d::CCScene::create();
    layer->setPosition({0.f, 0.f});
    scene->addChild(layer);

    return scene;
}

GlobedMenuLayer* GlobedMenuLayer::create() {
    auto ret = new GlobedMenuLayer;
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}
