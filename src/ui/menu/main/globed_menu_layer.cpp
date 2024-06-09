#include "globed_menu_layer.hpp"

#include "server_list_cell.hpp"
#include "kofi_popup.hpp"
#include <data/types/misc.hpp>
#include <managers/account.hpp>
#include <managers/admin.hpp>
#include <managers/central_server.hpp>
#include <managers/error_queues.hpp>
#include <managers/game_server.hpp>
#include <net/manager.hpp>
#include <ui/menu/room/room_popup.hpp>
#include <ui/menu/server_switcher/server_switcher_popup.hpp>
#include <ui/menu/settings/settings_layer.hpp>
#include <ui/menu/level_list/level_list_layer.hpp>
#include <ui/menu/admin/admin_popup.hpp>
#include <ui/menu/admin/admin_login_popup.hpp>
#include <ui/menu/credits/credits_popup.hpp>
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
    serverSwitcherButton = Build<CCSprite>::createSpriteName("accountBtn_myLevels_001.png")
        .intoMenuItem([](auto) {
            if (auto* popup = ServerSwitcherPopup::create()) {
                popup->m_noElasticity = true;
                popup->show();
            }
        })
        .scaleMult(1.15f)
        .id("btn-open-server-switcher"_spr)
        .parent(leftButtonMenu)
        .collect();

    // discord button
    discordButton = Build<CCSprite>::createSpriteName("gj_discordIcon_001.png")
        .scale(1.35f)
        .intoMenuItem([](auto) {
            geode::createQuickPopup("Open Discord", "Join our <cp>Discord</c> server?", "No", "Yes", [] (auto fl, bool btn2) {
                if (btn2)
                    geode::utils::web::openLinkInBrowser(globed::string<"discord">());
            });
        })
        .scaleMult(1.15f)
        .id("btn-open-discord"_spr)
        .parent(leftButtonMenu)
        .collect();

    // settings button
    settingsButton = Build<CCSprite>::createSpriteName("accountBtn_settings_001.png")
        .intoMenuItem([](auto) {
            util::ui::switchToScene(GlobedSettingsLayer::create());
        })
        .scaleMult(1.15f)
        .id("btn-open-settings"_spr)
        .parent(leftButtonMenu)
        .collect();

    // room popup button
    roomButton = Build<CCSprite>::createSpriteName("accountBtn_friends_001.png")
        .intoMenuItem([](auto) {
            // this->requestServerList();
            if (auto* popup = RoomPopup::create()) {
                popup->m_noElasticity = true;
                popup->show();
            }
        })
        .scaleMult(1.15f)
        .id("btn-refresh-servers"_spr)
        .collect();

    auto& settings = GlobedSettings::get();

    // level list button
    levelListButton = Build<CCSprite>::createSpriteName("icon-level-list.png"_spr)
        .intoMenuItem([](auto) {
            util::ui::switchToScene(GlobedLevelListLayer::create());
        })
        .scaleMult(1.15f)
        .id("btn-open-level-list"_spr)
        .collect();

    leftButtonMenu->updateLayout();

    // right button menu

    Build<CCMenu>::create()
        .layout(
            ColumnLayout::create()
                ->setAutoScale(true)
                ->setGap(4.f)
                ->setAxisAlignment(AxisAlignment::Start)
        )
        .anchorPoint(1.f, 0.f)
        .pos(winSize.width - 15.f, 20.f)
        .parent(this)
        .id("right-button-menu"_spr)
        .store(rightButtonMenu);

    // kofi button
    Build<CCSprite>::createSpriteName("icon-kofi.png"_spr)
        .intoMenuItem([](auto) {
            GlobedKofiPopup::create()->show();
        })
        .scaleMult(1.15f)
        .id("btn-kofi"_spr)
        .parent(rightButtonMenu)
        .intoNewChild(CCSprite::createWithSpriteFrameName("icon-kofi-glow.png"_spr))
        .zOrder(-1)
        .anchorPoint(0.f, 0.f)
        .pos(-3.f, -5.f)
        .scale(1.f);

    // credits button
    Build<CCSprite>::createSpriteName("icon-credits.png"_spr)
        .intoMenuItem([](auto) {
            auto* popup = GlobedCreditsPopup::create();
            popup->m_noElasticity = true;
            popup->show();
        })
        .scaleMult(1.15f)
        .id("btn-credits"_spr)
        .parent(rightButtonMenu);

    // info button
    if (settings.communication.voiceEnabled) {
        Build<CCSprite>::createSpriteName("icon-voice-chat-guide.png"_spr)
            .intoMenuItem([](auto) {
                FLAlertLayer::create(
                    "Voice chat guide",
    #ifdef GLOBED_VOICE_CAN_TALK
                    "In order to <cg>talk</c> with other people in-game, <cp>hold V</c>.\nIn order to <cr>deafen</c> (stop hearing everyone), <cb>press B</c>.\nBoth keybinds can be changed in <cy>Geometry Dash</c> settings.",
    #else
                    "This platform currently <cr>does not</c> support audio recording, but you can still hear others in voice chat. Sorry for the inconvenience.",
    #endif
                "Ok")->show();
            })
            .scaleMult(1.15f)
            .id("btn-show-voice-chat-popup"_spr)
            .parent(rightButtonMenu);
    }

    rightButtonMenu->updateLayout();

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

    // update ping of the active server, if any
    nm.updateServerPing();

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

    auto request = web::WebRequest()
        .userAgent(util::net::webUserAgent())
        .timeout(util::time::seconds(3))
        .get(fmt::format("{}/servers?protocol={}", centralUrl.value().url, NetworkManager::get().getUsedProtocol()))
        .map([this](web::WebResponse* response) -> Result<std::string, std::string> {
            GLOBED_UNWRAP_INTO(response->string(), auto resptext);

            if (resptext.empty()) {
                return Err(fmt::format("server sent an empty response with code {}", response->code()));
            }

            if (response->ok()) {
                return Ok(resptext);
            } else {
                return Err(fmt::format("code {}: {}", response->code(), resptext));
            }

        }, [](web::WebProgress*) -> std::monostate {
            return {};
        });

    requestListener.bind(this, &GlobedMenuLayer::requestCallback);
    requestListener.setFilter(std::move(request));
}

void GlobedMenuLayer::requestCallback(typename Task<Result<std::string, std::string>>::Event* event) {
    if (!event || !event->getValue()) return;

    if (event->getValue()->isErr()) {
        auto& gsm = GameServerManager::get();
        gsm.clearCache();
        gsm.clear();
        gsm.pendingChanges = true;

        ErrorQueues::get().error(fmt::format("Failed to fetch servers.\n\nReason: <cy>{}</c>", event->getValue()->unwrapErr()));

        return;
    }

    auto response = event->getValue()->unwrap();

    auto& gsm = GameServerManager::get();
    gsm.updateCache(response);
    auto result = gsm.loadFromCache();
    gsm.pendingChanges = true;

    if (result.isErr()) {
        log::warn("failed to parse server list: {}", result.unwrapErr());
        log::warn("{}", response);
        ErrorQueues::get().error(fmt::format("Failed to parse server list: <cy>{}</c>", result.unwrapErr()));
    }
}

void GlobedMenuLayer::cancelWebRequest() {
    requestListener.getFilter().cancel();
}

void GlobedMenuLayer::keyBackClicked() {
    util::ui::navigateBack();
}

void GlobedMenuLayer::keyDown(enumKeyCodes key) {
    if (key == enumKeyCodes::KEY_F8) {
        bool authorized = AdminManager::get().authorized();
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
    auto& nm = NetworkManager::get();
    nm.pingServers();
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
