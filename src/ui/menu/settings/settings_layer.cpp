#include "settings_layer.hpp"

#include "setting_header_cell.hpp"
#include "setting_cell.hpp"
#include "save_slot_switcher_popup.hpp"
#include "connection_test_popup.hpp"
#include <managers/settings.hpp>
#include <util/ui.hpp>
#include <util/cocos.hpp>

using namespace geode::prelude;
using SettingType = GlobedSettingCell::Type;

static constexpr float TAB_SCALE = 0.85f;

bool GlobedSettingsLayer::init(bool showConnectionTest) {
    if (!CCLayer::init()) return false;

    this->doShowConnectionTest = showConnectionTest;
    this->prevSettingsSlot = GlobedSettings::get().getSelectedSlot();
    this->setID("GlobedSettingsLayer"_spr);

    auto winsize = CCDirector::get()->getWinSize();

    auto* tabButtonMenu = Build<CCMenu>::create()
        .id("tab-button-menu")
        .zOrder(5)
        .layout(RowLayout::create()->setAutoScale(false)->setGap(-1.f))
        .pos(winsize.width / 2, winsize.height / 2 + LIST_HEIGHT / 2 + 26.f)
        .contentSize(LIST_WIDTH + 40.f, 0.f)
        .parent(this)
        .collect();

    // tab buttons
    Build<TabButton>::create("Globed", this, menu_selector(GlobedSettingsLayer::onTab))
        .scale(TAB_SCALE)
        .tag(TAG_TAB_GLOBED)
        .store(tabBtn1)
        .parent(tabButtonMenu);

    Build<TabButton>::create("Menus", this, menu_selector(GlobedSettingsLayer::onTab))
        .scale(TAB_SCALE)
        .tag(TAG_TAB_MENUS)
        .store(tabBtn2)
        .parent(tabButtonMenu);

    Build<TabButton>::create("Voice", this, menu_selector(GlobedSettingsLayer::onTab))
        .scale(TAB_SCALE)
        .tag(TAG_TAB_COMMUNICATION)
        .store(tabBtn3)
        .parent(tabButtonMenu);

    Build<TabButton>::create("Level UI", this, menu_selector(GlobedSettingsLayer::onTab))
        .scale(TAB_SCALE)
        .tag(TAG_TAB_LEVELUI)
        .store(tabBtn4)
        .parent(tabButtonMenu);

    Build<TabButton>::create("Players", this, menu_selector(GlobedSettingsLayer::onTab))
        .scale(TAB_SCALE)
        .tag(TAG_TAB_PLAYERS)
        .store(tabBtn5)
        .parent(tabButtonMenu);

    tabButtonMenu->updateLayout();

    // tabs gradient (copied from geode)

    tabsGradientNode = CCClippingNode::create();
    tabsGradientNode->setID("gradient-clipping-node");
    tabsGradientNode->setContentSize(this->getContentSize());
    tabsGradientNode->setAnchorPoint({0.5f, 0.5f});
    tabsGradientNode->ignoreAnchorPointForPosition(true);
    tabsGradientNode->setZOrder(4);
    tabsGradientNode->setInverted(false);
    tabsGradientNode->setAlphaThreshold(0.7f);

    tabsGradientSprite = CCSprite::createWithSpriteFrameName("tab-gradient.png"_spr);
    tabsGradientSprite->setPosition(tabButtonMenu->getPosition());
    tabsGradientNode->addChild(tabsGradientSprite);

    tabsGradientStencil = CCSprite::createWithSpriteFrameName("tab-gradient-mask.png"_spr);
    tabsGradientStencil->setScale(TAB_SCALE);
    tabsGradientStencil->setAnchorPoint({0.f, 0.f});
    tabsGradientNode->setStencil(tabsGradientStencil);

    this->addChild(tabsGradientNode);
    this->addChild(tabsGradientStencil);

    auto rightMenu = Build<CCMenu>::create()
        .layout(ColumnLayout::create()->setAxisAlignment(AxisAlignment::Start))
        .anchorPoint(1.f, 0.f)
        .pos(winsize.width - 8.f, 8.f)
        .contentSize(48.f, winsize.height)
        .id("right-side-menu")
        .parent(this)
        .collect();

    // Reset settings button
    auto resetBtn = Build<CCSprite>::createSpriteName("GJ_deleteBtn_001.png")
        .intoMenuItem([this](auto) {
            geode::createQuickPopup("Reset all settings", "Are you sure you want to reset all settings? This action is <cr>irreversible.</c>", "Cancel", "Ok", [this](auto, bool accepted) {
                if (accepted) {
                    GlobedSettings::get().reset();
                    this->remakeList();
                }
            });
        })
        .id("btn-reset")
        .parent(rightMenu)
        .collect();

    // Save slot button
    Build<CircleButtonSprite>::create(CCSprite::createWithSpriteFrameName("icon-folder-settings.png"_spr), CircleBaseColor::Pink)
        .with([&](auto* item) {
            util::ui::rescaleToMatch(item, resetBtn);
        })
        .intoMenuItem([this](auto) {
            SaveSlotSwitcherPopup::create()->show();
        })
        .id("btn-save-slots")
        .parent(rightMenu);

    rightMenu->updateLayout();

    util::ui::prepareLayer(this);

    this->remakeList();
    this->scheduleUpdate();

    return true;
}

void GlobedSettingsLayer::update(float dt) {
    if (GlobedSettings::get().getSelectedSlot() == this->prevSettingsSlot) {
        return;
    }

    this->prevSettingsSlot = GlobedSettings::get().getSelectedSlot();

    // yeah so alk would probably demote me from lead dev for this code but it is what it is
    // yeah im gonna kill you - lime

    auto newLayer = GlobedSettingsLayer::create(false);

    for (auto tab : storedTabs) {
        tab.second->removeFromParent();
    }

    // Idk why this is here but dont remove it or reorder it
    this->getParent()->addChild(newLayer);
    newLayer->removeFromParent();

    this->storedTabs = std::move(newLayer->storedTabs);
    this->settingCells = std::move(newLayer->settingCells);
    this->onTabById(currentTab);
}

void GlobedSettingsLayer::onTab(CCObject* sender) {
    for (const auto& [_, tab] : storedTabs) {
        if (tab->getParent()) {
            tab->removeFromParent();
        }
    }

    if (!storedTabs.contains(sender->getTag())) {
        return;
    }

    this->onTabById(sender->getTag());
}

void GlobedSettingsLayer::onTabById(int tag) {
    currentTab = tag;
    this->addChild(storedTabs[currentTab]);

    // i stole this from geode
    auto toggleTab = [this](CCMenuItemToggler* member) {
        auto isSelected = member->getTag() == currentTab;
        member->toggle(isSelected);

        if (isSelected && tabsGradientStencil) {
            tabsGradientStencil->setPosition(member->m_onButton->convertToWorldSpace({0.f, 0.f}));
        }
    };

    toggleTab(tabBtn1);
    toggleTab(tabBtn2);
    toggleTab(tabBtn3);
    toggleTab(tabBtn4);
    toggleTab(tabBtn5);

    cocos::handleTouchPriority(this, true);
}

void GlobedSettingsLayer::keyBackClicked() {
    util::ui::navigateBack();
}

void GlobedSettingsLayer::onEnterTransitionDidFinish() {
    if (!doShowConnectionTest) return;

    Loader::get()->queueInMainThread([] {
        ConnectionTestPopup::create()->show();
    });
}

void GlobedSettingsLayer::remakeList() {
    if (currentTab != -1 && storedTabs.contains(currentTab)) {
        storedTabs[currentTab]->removeFromParent();
    }

    currentTab = TAG_TAB_GLOBED;

    storedTabs.clear();

    storedTabs[TAG_TAB_GLOBED] = this->makeListLayer(TAG_TAB_GLOBED);
    storedTabs[TAG_TAB_MENUS] = this->makeListLayer(TAG_TAB_MENUS);
    storedTabs[TAG_TAB_COMMUNICATION] = this->makeListLayer(TAG_TAB_COMMUNICATION);
    storedTabs[TAG_TAB_LEVELUI] = this->makeListLayer(TAG_TAB_LEVELUI);
    storedTabs[TAG_TAB_PLAYERS] = this->makeListLayer(TAG_TAB_PLAYERS);

    this->onTab(tabBtn1);
}

template <typename T>
constexpr static GlobedSettingCell::Type getCellType() {
    using Type = GlobedSettingCell::Type;

    if constexpr (std::is_same_v<T, bool>) {
        return Type::Bool;
    } else if constexpr (std::is_same_v<T, float>) {
        return Type::Float;
    } else if constexpr (std::is_same_v<T, int>) {
        return Type::Int;
    } else {
        static_assert(std::is_same_v<T, void>, "invalid type for getCellType");
    }
}

template <typename SetTy>
static void registerSetting(
        CCArray* cells,
        SetTy& setting,
        const char* name,
        const char* description,
        SettingType stype = getCellType<typename SetTy::Type>()
) {
    GlobedSettingCell::Limits limits = {};

    // check for limits
    if constexpr (SetTy::IsLimited) {
        if constexpr (std::is_same_v<typename SetTy::Type, float>) {
            limits.floatMin = SetTy::Minimum;
            limits.floatMax = SetTy::Maximum;
        } else if constexpr (std::is_same_v<typename SetTy::Type, int>) {
            limits.intMin = SetTy::Minimum;
            limits.intMax = SetTy::Maximum;
        }
    }

    cells->addObject(GlobedSettingCell::create(&setting.ref(), stype, name, description, limits));
}

void GlobedSettingsLayer::createSettingsCells(int category) {
    using Type = SettingType;

    settingCells[category] = CCArray::create();

    auto& settings = GlobedSettings::get();
    auto& cat = settingCells[category];

    switch (category) {
        case TAG_TAB_GLOBED: {
            registerSetting(cat, settings.dummySetting, "Connection Test", "Test the connection for possible issues. It is recommended to try this if you are experiencing connection issues.", Type::ConnectionTest);
            registerSetting(cat, settings.globed.autoconnect, "Autoconnect", "Automatically connect to the last connected server on launch.");
            registerSetting(cat, settings.globed.preloadAssets, "Preload assets", "Increases the loading times but prevents most lagspikes in a level.");
            registerSetting(cat, settings.globed.deferPreloadAssets, "Defer preloading", "Instead of making the loading screen longer, load assets only when you join a level while connected.");
            registerSetting(cat, settings.globed.invitesFrom, "Receive invites from", "Controls who can invite you into a room.", Type::InvitesFrom);
            registerSetting(cat, settings.globed.editorSupport, "View players in editor", "Enables the ability to see people playing your level while in the editor. Note: <cy>this does not let you build levels together!</c>");
            registerSetting(cat, settings.dummySetting, "Keybinds", "Opens the <cg>Keybinds Settings</c>.", Type::KeybindSettings);
            registerSetting(cat, settings.globed.fragmentationLimit, "Packet limit", "Press the \"Test\" button to calibrate the maximum packet size. Should fix some of the issues with players not appearing in a level.", Type::PacketFragmentation);
            registerSetting(cat, settings.globed.forceTcp, "Force TCP", "Forces the use of TCP for <cg>all packets</c>. This may help with some connection issues, but it also may result in higher latency and less smooth gameplay. <cy>Reconnect to the server to see changes.</c>");
            registerSetting(cat, settings.globed.showRelays, "Show server relays", "Shows <cg>server relays</c> in the server list, if the current server has any. Relays can help if experiencing connection issues. Note: this feature is <cp>experimental</c>.");

#ifndef GEODE_IS_ANDROID
            registerSetting(cat, settings.globed.useDiscordRPC, "Discord RPC", "If you have the Discord Rich Presence standalone mod, this option will toggle a Globed-specific RPC on your profile.", Type::DiscordRPC);
#endif

            registerSetting(cat, settings.globed.changelogPopups, "Changelog Popup", "After every update, show a <cy>popup</c> detailing all the <cl>changes</c> in the <cg>update</c>.");
#ifdef GLOBED_GP_CHANGES
            registerSetting(cat, settings.globed.editorChanges, "Editor Global Triggers (Beta)", "<cp>NOTE: experimental, could cause level corruption.</c>\n\nChanges the UI of some triggers, adding new multiplayer functionality to them. A guide will be shown upon entering the editor.");
#endif

#ifdef GLOBED_DEBUG
            // advanced settings button
            registerSetting(cat, settings.dummySetting, "Advanced", "Advanced settings", Type::AdvancedSettings);
#endif
            // link code button
            registerSetting(cat, settings.dummySetting, "Discord link code", "Primarily for boosters/supporters/staff, code for linking an account to a Discord account", Type::LinkCode);
        } break;

        case TAG_TAB_MENUS: {
            registerSetting(cat, settings.globed.increaseLevelList, "More Levels Per Page", "Increases the levels per page in the server level list from 30 to 100.");
            registerSetting(cat, settings.globed.compressedPlayerCount, "Compressed Player Count", "Compress the Player Count label to match the Player Count in The Tower.");
        } break;

        case TAG_TAB_COMMUNICATION: {
#ifdef GLOBED_VOICE_SUPPORT
            registerSetting(cat, settings.communication.voiceEnabled, "Voice chat", "Enables in-game voice chat. To talk, hold V when in a level. (keybind can be changed in game settings)");
            registerSetting(cat, settings.communication.voiceProximity, "Voice proximity", "In platformer mode, the loudness of other players will be determined by how close they are to you.");
            registerSetting(cat, settings.communication.classicProximity, "Classic proximity", "Same as voice proximity, but for classic levels (non-platformer).");
            registerSetting(cat, settings.communication.voiceVolume, "Voice volume", "Controls how loud other players are.");
            registerSetting(cat, settings.communication.onlyFriends, "Only friends", "When enabled, you won't hear players that are not on your friend list in-game.");
            registerSetting(cat, settings.communication.lowerAudioLatency, "Lower audio latency", "Decreases the audio buffer size by 2 times, reducing the latency but potentially causing audio issues.");
            registerSetting(cat, settings.communication.tcpAudio, "TCP Voice chat", "Uses TCP instead of UDP for voice chat, may sometimes help with voice chat not working");
            registerSetting(cat, settings.communication.deafenNotification, "Deafen notification", "Shows a notification when you deafen & undeafen.");
            registerSetting(cat, settings.communication.audioDevice, "Audio device", "The input device used for recording your voice.", Type::AudioDevice);
            // MAKE_SETTING(communication, voiceLoopback, "Voice loopback", "When enabled, you will hear your own voice as you speak.");
#endif // GLOBED_VOICE_SUPPORT
        } break;

        case TAG_TAB_LEVELUI: {
            registerSetting(cat, settings.levelUi.progressIndicators, "Progress icons", "Show small icons under the progressbar (or at the edge of the screen in platformer), indicating how far other players are in the level.");
            registerSetting(cat, settings.levelUi.progressOpacity, "Indicator opacity", "Changes the opacity of the icons that represent other players.");
            registerSetting(cat, settings.levelUi.voiceOverlay, "Voice overlay", "Show a small overlay in the bottom right indicating currently speaking players.");
            registerSetting(cat, settings.levelUi.forceProgressBar, "Force Progress Bar", "Enables the progress bar when connected to Globed. Useful if you typically leave it off.");

            this->addHeader(category, "Ping overlay");
            registerSetting(cat, settings.overlay.enabled, "Enabled", "Show a small overlay when in a level, displaying the current latency to the server.");
            registerSetting(cat, settings.overlay.opacity, "Opacity", "Opacity of the displayed overlay.");
            registerSetting(cat, settings.overlay.hideConditionally, "Hide conditionally", "Hide the ping overlay when not connected to a server or in a non-uploaded level, instead of showing a substitute message.");
            registerSetting(cat, settings.overlay.position, "Position", "Position of the overlay on the screen.", Type::Corner);
        } break;

        case TAG_TAB_PLAYERS: {
            registerSetting(cat, settings.players.playerOpacity, "Opacity", "Opacity of other players.");
            registerSetting(cat, settings.players.showNames, "Player names", "Show names above players' icons.");
            registerSetting(cat, settings.players.dualName, "Dual name", "Show the name of the player on their secondary icon as well.");
            registerSetting(cat, settings.players.nameOpacity, "Name opacity", "Opacity of player names.");
            registerSetting(cat, settings.players.ownName, "Show own name", "Shows your own name above your icon as well.");
            registerSetting(cat, settings.players.rotateNames, "Rotate names", "Rotates player names with the camera rotation to keep them easily readable.");
            registerSetting(cat, settings.players.deathEffects, "Death effects", "Play a death effect whenever a player dies.");
            registerSetting(cat, settings.players.defaultDeathEffect, "Default death effect", "Replaces the death effects of all players with a default explosion effect.");
            registerSetting(cat, settings.players.hideNearby, "Hide nearby players", "Increases the transparency of players as they get closer to you, so that they don't obstruct your view.");
            registerSetting(cat, settings.players.statusIcons, "Status icons", "Show an icon above a player if they are paused, in practice mode, or currently speaking.");
            registerSetting(cat, settings.players.hidePracticePlayers, "Hide players in practice", "Hide players that are in practice mode.");
        } break;
    }
}

void GlobedSettingsLayer::addHeader(int category, const char* name) {
    settingCells[category]->addObject(GlobedSettingHeaderCell::create(name));
}

GJListLayer* GlobedSettingsLayer::makeListLayer(int category) {
    this->createSettingsCells(category);

    auto listview = Build<ListView>::create(settingCells[category], GlobedSettingCell::CELL_HEIGHT, LIST_WIDTH, LIST_HEIGHT)
        .collect();

    auto* listLayer = Build<GJListLayer>::create(listview, nullptr, globed::color::Brown, LIST_WIDTH, 220.f, 0)
        .zOrder(6)
        .anchorPoint(0.f, 0.f)
        .id(util::cocos::spr(fmt::format("setting-list-{}", category)).c_str())
        .collect();

    auto winSize = CCDirector::get()->getWinSize();
    listLayer->setPosition({winSize / 2 - listLayer->getScaledContentSize() / 2});

    return listLayer;
}

GlobedSettingsLayer* GlobedSettingsLayer::create(bool showConnectionTest) {
    auto ret = new GlobedSettingsLayer;
    if (ret->init(showConnectionTest)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}
