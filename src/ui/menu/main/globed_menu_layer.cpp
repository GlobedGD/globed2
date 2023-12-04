#include "globed_menu_layer.hpp"

#include <Geode/utils/web.hpp>
#include <UIBuilder.hpp>

#include "server_list_cell.hpp"
#include <ui/menu/player_list/player_list_popup.hpp>
#include <util/ui.hpp>
#include <util/net.hpp>
#include <net/network_manager.hpp>
#include <managers/server_manager.hpp>
#include <managers/error_queues.hpp>
#include <managers/account_manager.hpp>

using namespace geode::prelude;

bool GlobedMenuLayer::init() {
    if (!CCLayer::init()) return false;

    auto& am = GlobedAccountManager::get();
    auto* gjam = GJAccountManager::get();
    auto& sm = GlobedServerManager::get();

    am.initialize(gjam->m_username, gjam->m_accountID, gjam->getGJP(), sm.getCentral());

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

    // standalone dummy list

    auto saListview = Build<ListView>::create(createStandaloneList(), ServerListCell::CELL_HEIGHT, LIST_WIDTH, LIST_HEIGHT)
        .collect();

    Build<GJListLayer>::create(saListview, "Servers", ccc4(0, 0, 0, 180), LIST_WIDTH, 220.f)
        .zOrder(2)
        .anchorPoint(0.f, 0.f)
        .parent(this)
        .id("standalone-list"_spr)
        .store(standaloneLayer);

    standaloneLayer->setPosition({winsize / 2 - standaloneLayer->getScaledContentSize() / 2});

    Build<GlobedSignupLayer>::create()
        .zOrder(2)
        .anchorPoint(0.f, 0.f)
        .pos(listLayer->getPosition())
        .parent(this)
        .id("signup-layer"_spr)
        .store(signupLayer);

    // refresh servers btn

    Build<CCSprite>::createSpriteName("miniSkull_001.png")
        .scale(1.2f)
        .pos({-250.f, -70.f})
        .intoMenuItem([this](auto) {
            // this->requestServerList();
            if (auto* popup = PlayerListPopup::create()) {
                popup->m_noElasticity = true;
                popup->show();
            }
        })
        .intoNewParent(CCMenu::create())
        .id("btn-refresh-servers"_spr)
        .parent(this);

    // TODO prod remove wipe authtoken button

    // Build<CCSprite>::createSpriteName("d_skull01_001.png")
    //     .scale(1.2f)
    //     .pos(-250.f, -30.f)
    //     .intoMenuItem([this](auto) {
    //         GlobedAccountManager::get().clearAuthKey();
    //     })
    //     .intoNewParent(CCMenu::create())
    //     .id("btn-clear-authtoken")
    //     .parent(this);

    // TODO: menu for connecting to a standalone server directly with an IP and port
    // it must call the proper func in GlobedServerManager::addGameServer then try to NM::connectStandalone

    util::ui::addBackground(this);

    auto menu = CCMenu::create();
    this->addChild(menu);

    util::ui::addBackButton(this, menu, util::ui::navigateBack);

    this->setKeyboardEnabled(true);
    this->setKeypadEnabled(true);

    CCScheduler::get()->scheduleSelector(schedule_selector(GlobedMenuLayer::refreshServerList), this, 0.1f, false);
    CCScheduler::get()->scheduleSelector(schedule_selector(GlobedMenuLayer::pingServers), this, 5.0f, false);

    if (GlobedServerManager::get().gameServerCount() == 0) {
        this->requestServerList();
    }

    sm.pendingChanges = true; // force ping all servers
    this->refreshServerList(0.f);

    return true;
}

CCArray* GlobedMenuLayer::createServerList() {
    auto ret = CCArray::create();

    auto& nm = NetworkManager::get();
    auto& sm = GlobedServerManager::get();

    bool authenticated = nm.established();

    auto activeServer = sm.getActiveGameServer();

    for (const auto [serverId, server] : sm.extractGameServers()) {
        bool active = authenticated && serverId == activeServer;
        auto cell = ServerListCell::create(server, active);
        ret->addObject(cell);
    }

    return ret;
}

CCArray* GlobedMenuLayer::createStandaloneList() {
    auto ret = CCArray::create();

    GameServerView view = {};

    auto cell = ServerListCell::create(view, false);
    ret->addObject(cell);

    return ret;
}

void GlobedMenuLayer::refreshServerList(float _) {
    auto& am = GlobedAccountManager::get();
    auto& nm = NetworkManager::get();

    bool standalone = nm.established() && nm.standalone();

    // if we do not have a session token from the central server, and are not in a standalone server, don't show game servers
    if (!am.hasAuthKey() && !standalone) {
        listLayer->setVisible(false);
        standaloneLayer->setVisible(false);
        signupLayer->setVisible(true);
        return;
    }

    signupLayer->setVisible(false);

    if (standalone) {
        listLayer->setVisible(false);
        standaloneLayer->setVisible(true);

        // update the standalone cell
        auto listCells = standaloneLayer->m_listView->m_tableView->m_contentLayer->getChildren();
        if (listCells == nullptr) {
            return;
        }

        auto& sm = GlobedServerManager::get();

        auto* ccnodew = static_cast<CCNode*>(listCells->objectAtIndex(0));
        auto* slc = static_cast<ServerListCell*>(ccnodew->getChildren()->objectAtIndex(2));

        auto server = sm.getGameServer(GlobedServerManager::STANDALONE_SERVER_ID);
        slc->updateWith(server, true);
        return;
    }

    listLayer->setVisible(true);
    standaloneLayer->setVisible(false);

    // if there are pending changes, hard refresh the list and ping all servers
    auto& sm = GlobedServerManager::get();
    if (sm.pendingChanges) {
        sm.pendingChanges = false;

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

    auto active = sm.getActiveGameServer();

    bool authenticated = NetworkManager::get().established();

    for (auto* obj : CCArrayExt<CCNode>(listCells)) {
        auto slc = static_cast<ServerListCell*>(obj->getChildren()->objectAtIndex(2));
        auto server = sm.getGameServer(slc->gsview.id);
        slc->updateWith(server, authenticated && slc->gsview.id == active);
    }
}

void GlobedMenuLayer::requestServerList() {
    NetworkManager::get().disconnect(false);

    auto centralUrl = GlobedServerManager::get().getCentral();

    web::AsyncWebRequest()
        .userAgent(util::net::webUserAgent())
        .fetch(fmt::format("{}/servers", centralUrl))
        .json()
        .then([](json::Value response) {
            auto& sm = GlobedServerManager::get();
            sm.clearGameServers();
            sm.pendingChanges.store(true, std::memory_order::relaxed);

            try {
                auto serverList = response.as_array();
                for (const auto& obj : serverList) {
                    auto server = obj.as_object();
                    sm.addGameServer(
                        server["id"].as_string(),
                        server["name"].as_string(),
                        server["address"].as_string(),
                        server["region"].as_string()
                    );
                }
            } catch (const std::exception& e) {
                ErrorQueues::get().error("Failed to parse server list: <cy>{}</c>", e.what());
                sm.clearGameServers();
            }
        })
        .expect([](auto error) {
            ErrorQueues::get().error(fmt::format("Failed to fetch servers: <cy>{}</c>", error));

            auto& sm = GlobedServerManager::get();
            sm.clearGameServers();
            sm.pendingChanges.store(true, std::memory_order::relaxed);
        })
        .send();
}

void GlobedMenuLayer::keyBackClicked() {
    util::ui::navigateBack();
}

void GlobedMenuLayer::pingServers(float _) {
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