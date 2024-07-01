#include "globed_menu_layer.hpp"

#include "kofi_popup.hpp"
#include <ui/menu/featured/daily_popup.hpp>
#include <data/types/misc.hpp>
#include <managers/account.hpp>
#include <managers/admin.hpp>
#include <managers/central_server.hpp>
#include <managers/error_queues.hpp>
#include <managers/game_server.hpp>
#include <net/manager.hpp>
#include <ui/menu/room/room_layer.hpp>
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

    // daily menu

    Build<CCMenu>::create()
        .layout(
            ColumnLayout::create()
                ->setAutoScale(true)
                ->setGap(4.f)
                ->setAxisAlignment(AxisAlignment::Start)
        )
        .anchorPoint(0.f, 0.f)
        .pos(11.0f, winSize.height * 0.55f)
        .parent(this)
        .id("daily-button-menu"_spr)
        .store(dailyButtonMenu);

    auto makeSprite = [this]{
        return CircleButtonSprite::createWithSpriteFrameName(
            "icon-crown-btn.png"_spr,
            1.05f,
            CircleBaseColor::Green,
            CircleBaseSize::Medium
        );
    };

    auto dailyPopupButton = Build<CircleButtonSprite>(makeSprite())
            .scale(1.1f)
            .intoMenuItem([this](auto) {
                DailyPopup::create()->show();
            })
            .scaleMult(1.15f)
            .id("btn-daily-popup"_spr)
            .parent(dailyButtonMenu)
            .collect();

    CCSprite* dailyPopupNew = Build<CCSprite>::createSpriteName("newMusicIcon_001.png")
    .id("btn-daily-extra"_spr)
    .anchorPoint({0.5, 0.5})
    .pos({dailyPopupButton->getScaledContentWidth() * 0.85f, dailyPopupButton->getScaledContentHeight() * 0.15f})
    .parent(dailyPopupButton);
     auto newSequence = CCRepeatForever::create(CCSequence::create(
        CCEaseSineInOut::create(CCScaleTo::create(0.75f, 1.2f)),
        CCEaseSineInOut::create(CCScaleTo::create(0.75f, 1.0f)),
        nullptr
    ));
    dailyPopupNew->runAction(newSequence);

    dailyButtonMenu->updateLayout();

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
    CCMenuItemSpriteExtra* kofi = Build<CCSprite>::createSpriteName("icon-kofi.png"_spr)
        .intoMenuItem([](auto) {
            GlobedKofiPopup::create()->show();
        })
        .scaleMult(1.15f)
        .id("btn-kofi"_spr)
        .parent(rightButtonMenu);

    // kofi glow
    CCSprite* glow = Build<CCSprite>::createSpriteName("icon-glow.png"_spr)
    .id("btn-kofi-glow"_spr)
    .pos(kofi->getScaledContentSize() / 2)
    .zOrder(-1)
    .color({255, 255, 0})
    .opacity(200)
    .scale(0.93f)
    .blendFunc({GL_ONE, GL_ONE})
    .parent(kofi);
    glow->runAction(CCRepeatForever::create(CCSequence::create(CCFadeTo::create(1.5f, 50), CCFadeTo::create(1.5f, 200), nullptr)));

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
                    "Voice Chat Guide",
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

    auto rl = RoomLayer::create();
    if (rl) {
        Build(rl)
            .anchorPoint(0.5f, 0.5f)
            .pos(winSize / 2.f)
            .parent(this);
    }

    util::ui::prepareLayer(this);

    // background
    auto* bg = util::ui::makeRepeatingBackground("game_bg_01_001.png", {37, 50, 167});
    this->addChild(bg);
    this->background = bg;

    auto* backBtn = this->getChildByIDRecursive("back-button");

    if (backBtn) {
        // leave server btn
        auto* menu = backBtn->getParent();
        menu->setAnchorPoint({0.f, 1.f});
        menu->setPosition({5.f, winSize.height - 5.f});
        menu->setContentWidth(60.f);

        menu->setLayout(RowLayout::create()->setAxisAlignment(AxisAlignment::Start));

        Build<CCSprite>::createSpriteName("icon-leave-server.png"_spr)
            .scale(1.15f)
            .intoMenuItem([] {
                geode::createQuickPopup("Leave Server", "Are you sure you want to <cr>disconnect</c>\nfrom this server?", "Cancel", "Yes", [](auto, bool confirm) {
                    if (!confirm) return;

                    NetworkManager::get().disconnect();

                    auto layer = GlobedServersLayer::create();
                    util::ui::replaceScene(layer);
                });
            })
            .parent(menu)
            ;

        menu->updateLayout();
    }

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
        } else {
            levelListButton->removeFromParent();
        }

        leftButtonMenu->updateLayout();
    }

    if (!nm.established()) {
        this->navigateToServerLayer();
    }
}

void GlobedMenuLayer::keyBackClicked() {
    util::ui::navigateBack();
}

void GlobedMenuLayer::navigateToServerLayer() {
    auto* dir = CCDirector::get();

    if (typeinfo_cast<CCTransitionScene*>(dir->getRunningScene())) {
        return;
    }

    auto prevScene = static_cast<CCScene*>(dir->m_pobScenesStack->objectAtIndex(dir->m_pobScenesStack->count() - 2));

    if (!prevScene->getChildren() || prevScene->getChildrenCount() < 1) {
        this->keyBackClicked();
        return;
    }

    if (typeinfo_cast<GlobedServersLayer*>(prevScene->getChildren()->objectAtIndex(0))) {
        this->keyBackClicked();
        return;
    }

    // if the prev scene is a menulayer instead, replace scene to server layer
    util::ui::replaceScene(GlobedServersLayer::create());
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
