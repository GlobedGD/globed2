#include "globed_menu_layer.hpp"

#include "server_list_cell.hpp"
#include <ui/menu/room/room_popup.hpp>
#include <ui/menu/server_switcher/server_switcher_popup.hpp>
#include <ui/menu/settings/settings_layer.hpp>
#include <ui/menu/level_list/level_list_layer.hpp>
#include <ui/menu/admin/admin_popup.hpp>
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

    Build<GJListLayer>::create(listview, "Servers", util::ui::BG_COLOR_BROWN, LIST_WIDTH, 220.f, 0)
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

    Build<CCMenu>::create()
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
        .store(leftButtonMenu);

    // server switcher button
    Build<CCSprite>::createSpriteName("gj_folderBtn_001.png")
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
            util::ui::switchToScene(GlobedSettingsLayer::create());
        })
        .id("btn-open-settings"_spr)
        .parent(leftButtonMenu);

    // room popup button
    roomButton = Build<CCSprite>::createSpriteName("GJ_profileButton_001.png")
        .scale(0.875f)
        .intoMenuItem([](auto) {
            // this->requestServerList();
            if (auto* popup = RoomPopup::create()) {
                popup->m_noElasticity = true;
                popup->show();
            }
        })
        .id("btn-refresh-servers"_spr)
        .collect();

    // level list button
    levelListButton = Build<CCSprite>::createSpriteName("GJ_menuBtn_001.png")
        .scale(0.8f)
        .intoMenuItem([](auto) {
            util::ui::switchToScene(GlobedLevelListLayer::create());
        })
        .id("btn-open-level-list"_spr)
        .collect();

    leftButtonMenu->updateLayout();

    util::ui::prepareLayer(this);

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
    auto& nm = NetworkManager::get();

    // also update the buttons
    if (nm.established() != currentlyShowingButtons) {
        currentlyShowingButtons = nm.established();

        if (currentlyShowingButtons) {
            leftButtonMenu->addChild(levelListButton);
            leftButtonMenu->addChild(roomButton);
        } else {
            levelListButton->removeFromParent();
            roomButton->removeFromParent();
        }

        leftButtonMenu->updateLayout();
    }

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
        .timeout(util::time::seconds(3))
        .get(fmt::format("{}/servers", centralUrl.value().url))
        .text()
        .then([this](std::string& response) {
            this->serverRequestHandle = std::nullopt;
            auto& gsm = GameServerManager::get();
            gsm.updateCache(response);
            auto result = gsm.loadFromCache();
            if (result.isErr()) {
                ErrorQueues::get().error(fmt::format("Failed to parse server list: {}", result.unwrapErr()));
            }
            gsm.pendingChanges = true;
        })
        .expect([this](std::string error) {
            this->serverRequestHandle = std::nullopt;
            ErrorQueues::get().error(fmt::format("Failed to fetch servers: <cy>{}</c>", error));

            auto& gsm = GameServerManager::get();
            gsm.clearCache();
            gsm.clear();
            gsm.pendingChanges = true;
        })
        .send();
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

void GlobedMenuLayer::keyDown(enumKeyCodes key) {
    if (key == enumKeyCodes::KEY_F8) {
        AdminPopup::create()->show();
    } else {
        CCLayer::keyDown(key);
    }
}

void GlobedMenuLayer::pingServers(float) {
    NetworkManager::get().taskPingServers();
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
