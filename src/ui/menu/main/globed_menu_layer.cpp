#include "globed_menu_layer.hpp"

#include "kofi_popup.hpp"
#include <data/types/misc.hpp>
#include <managers/account.hpp>
#include <managers/admin.hpp>
#include <managers/central_server.hpp>
#include <managers/error_queues.hpp>
#include <managers/game_server.hpp>
#include <net/manager.hpp>
#include <ui/menu/room/room_popup.hpp>
#include <ui/menu/settings/settings_layer.hpp>
#include <ui/menu/servers/server_layer.hpp>
#include <ui/menu/level_list/level_list_layer.hpp>
#include <ui/menu/admin/admin_popup.hpp>
#include <ui/menu/admin/admin_login_popup.hpp>
#include <ui/menu/credits/credits_popup.hpp>
#include <util/net.hpp>
#include <util/format.hpp>
#include <util/ui.hpp>

using namespace geode::prelude;

bool GlobedMenuLayer::init() {
    if (!CCLayer::init()) return false;

    auto winSize = CCDirector::get()->getWinSize();

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
            auto layer = GlobedServersLayer::create();
            util::ui::switchToScene(layer);
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
        .parent(rightButtonMenu);

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

    Build<GJListLayer>::create(ListView::create(CCArray::create()), "thing", util::ui::BG_COLOR_BROWN, LIST_WIDTH, 220.f, 0)
        .zOrder(2)
        .with([&](auto* layer) {
            layer->setPosition(winSize / 2 - layer->getScaledContentSize() / 2);
        })
        .parent(this);

    util::ui::prepareLayer(this);

    this->scheduleUpdate();

    return true;
}

void GlobedMenuLayer::update(float dt) {
    auto& nm = NetworkManager::get();

    // update the buttons
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

GlobedMenuLayer* GlobedMenuLayer::create() {
    auto ret = new GlobedMenuLayer;
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}
