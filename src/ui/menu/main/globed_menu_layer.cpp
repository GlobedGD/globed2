#include "globed_menu_layer.hpp"

#include "server_list_cell.hpp"
#include <data/types/misc.hpp>
#include <managers/error_queues.hpp>
#include <managers/account.hpp>
#include <managers/central_server.hpp>
#include <managers/game_server.hpp>
#include <net/network_manager.hpp>
#include <ui/menu/room/room_popup.hpp>
#include <ui/menu/server_switcher/server_switcher_popup.hpp>
#include <ui/menu/settings/settings_layer.hpp>
#include <ui/menu/level_list/level_list_layer.hpp>
#include <ui/menu/admin/admin_popup.hpp>
#include <ui/menu/admin/admin_login_popup.hpp>
#include <util/ui.hpp>
#include <util/net.hpp>

using namespace geode::prelude;

bool GlobedMenuLayer::init() {
    if (!CCLayer::init()) return false;

    GlobedAccountManager::get().autoInitialize();

    auto winSize = CCDirector::get()->getWinSize();

    // server list

    auto listview = Build<ListView>::create(createServerList(), ServerListCell::CELL_HEIGHT, LIST_WIDTH, LIST_HEIGHT)
        .collect();

    Build<GJListLayer>::create(listview, "Servers", util::ui::BG_COLOR_BROWN, LIST_WIDTH, 220.f, 0)
        .zOrder(2)
        .anchorPoint(0.f, 0.f)
        .parent(this)
        .id("server-list"_spr)
        .store(listLayer);

    listLayer->setPosition({winSize / 2 - listLayer->getScaledContentSize() / 2});

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
                ->setGap(4.f)
                ->setAxisAlignment(AxisAlignment::Start)
        )
        .anchorPoint(0.f, 0.f)
        .pos(15.f, 20.f)
        .parent(this)
        .id("left-button-menu"_spr)
        .store(leftButtonMenu);

    // server switcher button
    serverSwitcherButton = Build<CCSprite>::createSpriteName("icon-server-folder.png"_spr)
        .intoMenuItem([](auto) {
            if (auto* popup = ServerSwitcherPopup::create()) {
                popup->m_noElasticity = true;
                popup->show();
            }
        })
        .id("btn-open-server-switcher"_spr)
        .parent(leftButtonMenu)
        .collect();
    serverSwitcherButton->m_scaleMultiplier = 1.15f;

    // discord button
    discordButton = Build<CCSprite>::createSpriteName("gj_discordIcon_001.png")
        .scale(1.35f)
        .intoMenuItem([](auto) {
            geode::createQuickPopup("Open Discord", "Join our <cp>Discord</c> server?", "No", "Yes", [] (auto fl, bool btn2) {
                if (btn2)
                    geode::utils::web::openLinkInBrowser("https://discord.gg/d56q5Dkdm3");
            });
        })
        .id("btn-open-discord"_spr)
        .parent(leftButtonMenu)
        .collect();
    discordButton->m_scaleMultiplier = 1.15f;

    // settings button
    settingsButton = Build<CCSprite>::createSpriteName("accountBtn_settings_001.png")
        .intoMenuItem([](auto) {
            util::ui::switchToScene(GlobedSettingsLayer::create());
        })
        .id("btn-open-settings"_spr)
        .parent(leftButtonMenu)
        .collect();
    settingsButton->m_scaleMultiplier = 1.15f;

    // room popup button
    roomButton = Build<CCSprite>::createSpriteName("accountBtn_friends_001.png")
        .intoMenuItem([](auto) {
            // this->requestServerList();
            if (auto* popup = RoomPopup::create()) {
                popup->m_noElasticity = true;
                popup->show();
            }
        })
        .id("btn-refresh-servers"_spr)
        .collect();
    roomButton->m_scaleMultiplier = 1.15f;

    // level list button
    levelListButton = Build<CCSprite>::createSpriteName("icon-level-list.png"_spr)
        .intoMenuItem([](auto) {
            util::ui::switchToScene(GlobedLevelListLayer::create());
        })
        .id("btn-open-level-list"_spr)
        .collect();
    levelListButton->m_scaleMultiplier = 1.15f;

    leftButtonMenu->updateLayout();

    // info button
    Build<CCSprite>::createSpriteName("GJ_infoIcon_001.png")
        .scale(1.0f)
        .intoMenuItem([](auto) {
            FLAlertLayer::create("Voice chat guide",
            "In order to <cg>talk</c> with other people in-game, <cp>hold V</c>.\nIn order to <cr>deafen</c> (stop hearing everyone), <cb>press B</c>.\nBoth keybinds can be changed in <cy>Geometry Dash</c> settings.",
            "Ok")->show();
        })
        .id("btn-show-voice-chat-popup"_spr)
        .pos(winSize.width - 20.f, 20.f)
        .intoNewParent(CCMenu::create())
        .pos(0.f, 0.f)
        .parent(this);

    util::ui::prepareLayer(this);

    this->schedule(schedule_selector(GlobedMenuLayer::refreshServerList), 0.1f);
    this->schedule(schedule_selector(GlobedMenuLayer::pingServers), 5.0f);

    auto& gsm = GameServerManager::get();

    this->requestServerList();

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

        // also disconnect from the current server if it's gone
        auto activeId = gsm.getActiveId();
        if (!gsm.getServer(activeId).has_value()) {
            NetworkManager::get().disconnect(false);
        }

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
                log::warn("failed to parse server list: {}", result.unwrapErr());
                log::warn("{}", response);
                ErrorQueues::get().error(fmt::format("Failed to parse server list: <cy>{}</c>", result.unwrapErr()));
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
        bool authorized = NetworkManager::get().isAuthorizedAdmin();
        if (authorized) {
            AdminPopup::create()->show();
        } else {
            AdminLoginPopup::create()->show();
        }
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
