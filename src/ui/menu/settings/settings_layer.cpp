#include "settings_layer.hpp"

#include <util/ui.hpp>
#include <managers/settings.hpp>

using namespace geode::prelude;

static constexpr float TAB_SCALE = 0.85f;

bool GlobedSettingsLayer::init() {
    if (!CCLayer::init()) return false;

    auto winsize = CCDirector::get()->getWinSize();

    auto* tabButtonMenu = Build<CCMenu>::create()
        .zOrder(5)
        .layout(RowLayout::create()->setAutoScale(false)->setGap(-16.f)) // TODO set to -1.f for new index
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

    Build<TabButton>::create("Overlay", this, menu_selector(GlobedSettingsLayer::onTab))
        .scale(TAB_SCALE)
        .tag(TAG_TAB_OVERLAY)
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
    tabsGradientNode->setContentSize(this->getContentSize());
    tabsGradientNode->setAnchorPoint({0.5f, 0.5f});
    tabsGradientNode->ignoreAnchorPointForPosition(true);
    tabsGradientNode->setZOrder(4);
    tabsGradientNode->setInverted(false);
    tabsGradientNode->setAlphaThreshold(0.7f);

    tabsGradientSprite = CCSprite::create("geode.loader/tab-gradient.png");
    tabsGradientSprite->setPosition(tabButtonMenu->getPosition());
    tabsGradientNode->addChild(tabsGradientSprite);

    tabsGradientStencil = CCSprite::create("geode.loader/tab-gradient-mask.png");
    tabsGradientStencil->setScale(TAB_SCALE);
    tabsGradientStencil->setAnchorPoint({0.f, 0.f});
    tabsGradientNode->setStencil(tabsGradientStencil);

    this->addChild(tabsGradientNode);
    this->addChild(tabsGradientStencil);

    Build<CCSprite>::createSpriteName("GJ_deleteBtn_001.png")
        .intoMenuItem([this](auto) {
            geode::createQuickPopup("Reset all settings", "Are you sure you want to reset all settings? This action is <cr>irreversible.</c>", "Cancel", "Ok", [this](auto, bool accepted) {
                if (accepted) {
                    GlobedSettings::get().resetToDefaults();
                    this->remakeList();
                }
            });
        })
        .pos(winsize.width - 30.f, 30.f)
        .intoNewParent(CCMenu::create())
        .pos(0.f, 0.f)
        .parent(this);

    util::ui::prepareLayer(this);

    this->remakeList();

    return true;
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

    currentTab = sender->getTag();
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

void GlobedSettingsLayer::remakeList() {
    if (currentTab != -1 && storedTabs.contains(currentTab)) {
        storedTabs[currentTab]->removeFromParent();
    }

    currentTab = TAG_TAB_GLOBED;

    storedTabs.clear();

    storedTabs[TAG_TAB_GLOBED] = this->makeListLayer(TAG_TAB_GLOBED);
    storedTabs[TAG_TAB_OVERLAY] = this->makeListLayer(TAG_TAB_OVERLAY);
    storedTabs[TAG_TAB_COMMUNICATION] = this->makeListLayer(TAG_TAB_COMMUNICATION);
    storedTabs[TAG_TAB_LEVELUI] = this->makeListLayer(TAG_TAB_LEVELUI);
    storedTabs[TAG_TAB_PLAYERS] = this->makeListLayer(TAG_TAB_PLAYERS);

    this->onTab(tabBtn1);
}

#define MAKE_SETTING(cat, name, setname, setdesc) cells->addObject(GlobedSettingCell::create(&settings.cat.name, getCellType<decltype(settings.cat.name)>(), setname, setdesc, {}))
#define MAKE_SETTING_TYPE(cat, name, type, setname, setdesc) cells->addObject(GlobedSettingCell::create(&settings.cat.name, type, setname, setdesc, {}))
#define MAKE_SETTING_LIM(cat, name, setname, setdesc, ...) cells->addObject(GlobedSettingCell::create(&settings.cat.name, getCellType<decltype(settings.cat.name)>(), setname, setdesc, __VA_ARGS__))
#define MAKE_HEADER(name) cells->addObject(GlobedSettingHeaderCell::create(name))

CCArray* GlobedSettingsLayer::createSettingsCells(int category) {
    using Type = GlobedSettingCell::Type;

    auto cells = CCArray::create();

    auto& settings = GlobedSettings::get();

    if (category == TAG_TAB_GLOBED) {
        MAKE_SETTING(globed, autoconnect, "Autoconnect", "Automatically connect to the last connected server on launch.");
        MAKE_SETTING(globed, preloadAssets, "Preload assets", "Increases the loading times but prevents most lagspikes in a level.");
        MAKE_SETTING(globed, deferPreloadAssets, "Defer preloading", "Instead of making the loading screen longer, load assets only when you join a level while connected.");
        MAKE_SETTING_TYPE(globed, fragmentationLimit, Type::PacketFragmentation, "Packet limit", "Press the \"Test\" button to calibrate the maximum packet size. Should fix some of the issues with players not appearing in a level.");
        MAKE_SETTING(globed, increaseLevelList, "More Levels Per Page", "Increases the levels per page in the server level list from 30 to 100.");
        MAKE_SETTING_LIM(globed, tpsCap, "TPS cap", "Maximum amount of packets per second sent between the client and the server. Useful only for very silly things.", {
            .intMin = 1,
            .intMax = 240,
        });
    }

    if (category == TAG_TAB_OVERLAY) {
        MAKE_SETTING(overlay, enabled, "Ping overlay", "Show a small overlay when in a level, displaying the current latency to the server.");
        MAKE_SETTING_LIM(overlay, opacity, "Overlay opacity", "Opacity of the displayed overlay.", {
            .floatMin = 0.f,
            .floatMax = 1.f
        });
        MAKE_SETTING(overlay, hideConditionally, "Hide conditionally", "Hide the ping overlay when not connected to a server or in a non-uploaded level, instead of showing a substitute message.");
        MAKE_SETTING_TYPE(overlay, position, Type::Corner, "Position", "Position of the overlay on the screen.");
    }

#ifdef GLOBED_VOICE_SUPPORT
    if (category == TAG_TAB_COMMUNICATION) {
        MAKE_SETTING(communication, voiceEnabled, "Voice chat", "Enables in-game voice chat. To talk, hold V when in a level. (keybind can be changed in game settings)");
        MAKE_SETTING(communication, voiceProximity, "Voice proximity", "In platformer mode, the loudness of other players will be determined by how close they are to you.");
        MAKE_SETTING(communication, classicProximity, "Classic proximity", "Same as voice proximity, but for classic levels (non-platformer).");
        MAKE_SETTING_LIM(communication, voiceVolume, "Voice volume", "Controls how loud other players are.", {
            .floatMin = 0.f,
            .floatMax = 2.f,
        });
        MAKE_SETTING(communication, onlyFriends, "Only friends", "When enabled, you won't hear players that are not on your friend list in-game.");
        MAKE_SETTING(communication, lowerAudioLatency, "Lower audio latency", "Decreases the audio buffer size by 2 times, reducing the latency but potentially causing audio issues.");
        MAKE_SETTING(communication, deafenNotification, "Deafen notification", "Shows a notification when you deafen & undeafen.");
        MAKE_SETTING_TYPE(communication, audioDevice, Type::AudioDevice, "Audio device", "The input device used for recording your voice.");
        // MAKE_SETTING(communication, voiceLoopback, "Voice loopback", "When enabled, you will hear your own voice as you speak.");
    }
#endif // GLOBED_VOICE_SUPPORT

    if (category == TAG_TAB_LEVELUI) {
        MAKE_SETTING(levelUi, progressIndicators, "Progress icons", "Show small icons under the progressbar (or at the edge of the screen in platformer), indicating how far other players are in the level.");
        MAKE_SETTING_LIM(levelUi, progressOpacity, "Indicator opacity", "Changes the opacity of the icons that represent other players.", {
            .floatMin = 0.f,
            .floatMax = 1.f
        });
        MAKE_SETTING(levelUi, voiceOverlay, "Voice overlay", "Show a small overlay in the bottom right indicating currently speaking players.");
    }

    if (category == TAG_TAB_PLAYERS) {
        MAKE_SETTING_LIM(players, playerOpacity, "Opacity", "Opacity of other players.", {
            .floatMin = 0.f,
            .floatMax = 1.f,
        });
        MAKE_SETTING(players, showNames, "Player names", "Show names above players' icons.");
        MAKE_SETTING(players, dualName, "Dual name", "Show the name of the player on their secondary icon as well.");
        MAKE_SETTING_LIM(players, nameOpacity, "Name opacity", "Opacity of player names.", {
            .floatMin = 0.f,
            .floatMax = 1.f
        });
        MAKE_SETTING(players, ownName, "Show own name", "Shows your own name above your icon as well.");
        MAKE_SETTING(players, deathEffects, "Death effects", "Play a death effect whenever a player dies.");
        MAKE_SETTING(players, defaultDeathEffect, "Default death effect", "Replaces the death effects of all players with a default explosion effect.");
        MAKE_SETTING(players, hideNearby, "Hide nearby players", "Increases the transparency of players as they get closer to you, so that they don't obstruct your view.");
        MAKE_SETTING(players, statusIcons, "Status icons", "Show an icon above a player if they are paused, in practice mode, or currently speaking.");
    }

    return cells;
}

GJListLayer* GlobedSettingsLayer::makeListLayer(int category) {
    auto listview = Build<ListView>::create(createSettingsCells(category), GlobedSettingCell::CELL_HEIGHT, LIST_WIDTH, LIST_HEIGHT)
        .collect();

    auto* listLayer = Build<GJListLayer>::create(listview, nullptr, util::ui::BG_COLOR_BROWN, LIST_WIDTH, 220.f, 0)
        .zOrder(6)
        .anchorPoint(0.f, 0.f)
        .id(Mod::get()->expandSpriteName(fmt::format("setting-list-{}", category).c_str()))
        .collect();

    auto winSize = CCDirector::get()->getWinSize();
    listLayer->setPosition({winSize / 2 - listLayer->getScaledContentSize() / 2});

    return listLayer;
}

GlobedSettingsLayer* GlobedSettingsLayer::create() {
    auto ret = new GlobedSettingsLayer;
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}
