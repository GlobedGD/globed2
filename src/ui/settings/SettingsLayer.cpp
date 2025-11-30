#include "SettingsLayer.hpp"
#include "AudioDeviceSetupPopup.hpp"
#include "cells/BoolSettingCell.hpp"
#include "cells/FloatSettingCell.hpp"
#include "cells/TitleSettingCell.hpp"
#include "cells/ButtonSettingCell.hpp"
#include "cells/IntSliderSettingCell.hpp"
#include "cells/IntCornerSettingCell.hpp"
#include "DiscordLinkPopup.hpp"
#include "KeybindsPopup.hpp"
#include "SaveSlotSwitcherPopup.hpp"
#include <globed/core/PopupManager.hpp>
#include <core/net/NetworkManagerImpl.hpp>
#include <ui/misc/InputPopup.hpp>

#include <UIBuilder.hpp>
#include <argon/argon.hpp>

using namespace geode::prelude;

namespace globed {

bool SettingsLayer::init() {
    BaseLayer::init(true);

    auto winSize = CCDirector::get()->getWinSize();

    for (size_t i = 0; i < 5; ++i) {
        auto tab = Build(cue::ListNode::create({356.f, 220.f}, cue::Brown, cue::ListBorderStyle::SlimLevels))
            .zOrder(6)
            .pos(winSize / 2.f)
            .parent(this)
            .visible(false)
            .collect();

        m_tabs.push_back(tab);
        tab->setAutoUpdate(false);
    }

    m_globedTab = m_tabs[0];
    m_playersTab = m_tabs[1];
    m_levelUiTab = m_tabs[2];
    m_voiceTab = m_tabs[3];
    m_menusTab = m_tabs[4];

    constexpr static auto names = std::array{
        "Globed",
        "Players",
        "Level UI",
        "Chat",
        "Menus",
    };

    this->addSettings();

    auto rightMenu = Build<CCMenu>::create()
        .layout(ColumnLayout::create()->setAxisAlignment(AxisAlignment::Start))
        .anchorPoint(1.f, 0.f)
        .pos(winSize.width - 8.f, 8.f)
        .contentSize(48.f, winSize.height)
        .id("right-side-menu")
        .parent(this)
        .collect();

    // Reset settings button
    auto resetBtn = Build<CCSprite>::createSpriteName("GJ_deleteBtn_001.png")
        .intoMenuItem([this](auto) {
            globed::confirmPopup(
                "Reset all settings",
                "Are you sure you want to reset all settings? This action is <cr>irreversible.</c>",
                "Cancel", "Ok",
                [this](auto) {
                    SettingsManager::get().reset();
                    this->refreshAll();
                }
            );
        })
        .id("btn-reset")
        .parent(rightMenu)
        .collect();

    // Save slot button
    Build<CircleButtonSprite>::create(CCSprite::create("icon-folder-settings.png"_spr), CircleBaseColor::Pink)
        .with([&](auto* item) { cue::rescaleToMatch(item, resetBtn); })
        .intoMenuItem([this](auto) {
            auto popup = SaveSlotSwitcherPopup::create();
            popup->setSwitchCallback([this] {
                this->refreshAll();
            });
            popup->show();
        })
        .id("btn-save-slots")
        .parent(rightMenu);

    rightMenu->updateLayout();

    // Tab switcher buttons

    auto tabButtonMenu = Build<CCMenu>::create()
        .id("tab-button-menu")
        .zOrder(5)
        .layout(RowLayout::create()->setAutoScale(false)->setGap(-1.f))
        .pos(winSize.width / 2, winSize.height / 2 + 220.f / 2 + 26.f)
        .contentSize(356.f + 40.f, 0.f)
        .parent(this)
        .collect();

    for (size_t i = 0; i < m_tabs.size(); i++) {
        auto tab = m_tabs[i];

        auto btn = Build<TabButton>::create(names[i], this, menu_selector(SettingsLayer::onTabClick))
            .scale(0.8f)
            .tag(i)
            .parent(tabButtonMenu)
            .collect();

        m_tabButtons.push_back(btn);
    }

    tabButtonMenu->updateLayout();

    // tab gradient

    m_tabsClipper = CCClippingNode::create();
    m_tabsClipper->setID("gradient-clipping-node");
    m_tabsClipper->setContentSize(this->getContentSize());
    m_tabsClipper->setAnchorPoint({0.5f, 0.5f});
    m_tabsClipper->ignoreAnchorPointForPosition(true);
    m_tabsClipper->setZOrder(4);
    m_tabsClipper->setInverted(false);
    m_tabsClipper->setAlphaThreshold(0.7f);

    m_tabsGradientSpr = CCSprite::create("tab-gradient.png"_spr);
    m_tabsGradientSpr->setPosition(tabButtonMenu->getPosition());
    m_tabsClipper->addChild(m_tabsGradientSpr);

    m_tabsGradientStencil = CCSprite::create("tab-gradient-mask.png"_spr);
    m_tabsGradientStencil->setScale(0.8f);
    m_tabsGradientStencil->setAnchorPoint({0.f, 0.f});
    m_tabsClipper->setStencil(m_tabsGradientStencil);

    this->addChild(m_tabsClipper);
    this->addChild(m_tabsGradientStencil);

    this->selectTab(m_globedTab);

    return true;
}

void SettingsLayer::addSettings() {
#ifdef GLOBED_DEBUG
    bool showDebug = true;
#else
    bool showDebug = globed::value<bool>("core.dev.enable-dev-settings").value_or(false);
#endif

    // TODO: descriptions
    // Globed
    this->addHeader("core", "General", m_globedTab);
    this->addSetting<BoolSettingCell>("core.autoconnect", "Autoconnect", "");
    this->addSetting(ButtonSettingCell::create("Discord Linking", "", "Link", [] {
        if (!NetworkManagerImpl::get().isConnected()) {
            globed::alert("Error", "Cannot do this while not connected to a server.");
            return;
        }

        DiscordLinkPopup::create()->show();
    }, CELL_SIZE));
    this->addSetting(ButtonSettingCell::create("Keybinds", "", "Edit", [] {
        KeybindsPopup::create()->show();
    }, CELL_SIZE));

    // Preload
    this->addHeader("core.player", "Preloading", m_globedTab);
    this->addSetting<BoolSettingCell>("core.preload.enabled", "Preload Assets", "");
    this->addSetting<BoolSettingCell>("core.preload.defer", "Defer Preloading", "");

    // Advanced settings
    this->addHeader("core.dev", "Advanced", m_globedTab);
    this->addSetting<BoolSettingCell>("core.ui.allow-custom-servers", "Allow Custom Servers", "");

    if (showDebug) {
        this->addSetting<FloatSettingCell>("core.dev.packet-loss-sim", "Packet Loss Simulation", "");
        this->addSetting<BoolSettingCell>("core.dev.net-debug-logs", "Network Debug Logs", "");
        this->addSetting<BoolSettingCell>("core.dev.net-stat-dump", "Network Stat Dump", "");
        this->addSetting<BoolSettingCell>("core.dev.fake-data", "Use Fake Data", "");

        this->addSetting(ButtonSettingCell::create("Clear Auth Tokens", "", "Clear", [] {
            NetworkManagerImpl::get().clearAllUTokens();
        }, CELL_SIZE));

        this->addSetting(ButtonSettingCell::create("Clear Argon Token", "", "Clear", [] {
            argon::clearToken();
        }, CELL_SIZE));
    }

    // Player settings
    this->addHeader("core.player", "Players", m_playersTab);
    this->addSetting<FloatSettingCell>("core.player.opacity", "Player Opacity", "");
    this->addSetting<BoolSettingCell>("core.player.quick-chat-enabled", "Quick Chat", "");
    this->addSetting<BoolSettingCell>("core.player.show-names", "Player Names", "");
    this->addSetting<BoolSettingCell>("core.player.dual-name", "Player Dual Names", "");
    this->addSetting<FloatSettingCell>("core.player.name-opacity", "Name Opacity", "");
    this->addSetting<BoolSettingCell>("core.player.force-visibility", "Force Visibility", "");
    this->addSetting<BoolSettingCell>("core.player.hide-nearby-classic", "Hide Nearby (Classic)", "");
    this->addSetting<BoolSettingCell>("core.player.hide-nearby-plat", "Hide Nearby (Plat)", "");
    this->addSetting<BoolSettingCell>("core.player.hide-practicing", "Hide Practicing Players", "");
    this->addSetting<BoolSettingCell>("core.player.status-icons", "Show Status Icons", "");
    this->addSetting<BoolSettingCell>("core.player.rotate-names", "Rotate Names", "");
    this->addSetting<BoolSettingCell>("core.player.death-effects", "Death Effects", "");
    this->addSetting<BoolSettingCell>("core.player.default-death-effects", "Default Death Effects", "");

    // Level UI
    this->addHeader("core.level", "Level UI", m_levelUiTab);
    this->addSetting<BoolSettingCell>("core.level.progress-indicators", "Progress Icons (Classic)", "");
    this->addSetting<BoolSettingCell>("core.level.progress-indicators-plat", "Progress Icons (Plat)", "");
    this->addSetting<FloatSettingCell>("core.level.progress-opacity", "Progress Opacity", "");
    this->addSetting<BoolSettingCell>("core.level.voice-overlay", "Voice Chat Overlay", "");
    this->addSetting<BoolSettingCell>("core.level.force-progressbar", "Force progressbar", "");
    this->addSetting<BoolSettingCell>("core.level.self-status-icons", "Show Own Status Icons", "");
    this->addSetting<BoolSettingCell>("core.level.self-name", "Show Own Name", "");

    // Overlay
    this->addHeader("core.overlay", "Ping overlay", m_levelUiTab);
    this->addSetting<BoolSettingCell>("core.overlay.enabled", "Enable Overlay", "");
    this->addSetting<FloatSettingCell>("core.overlay.opacity", "Overlay Opacity", "");
    this->addSetting<IntCornerSettingCell>("core.overlay.position", "Overlay Position", "");
    this->addSetting<BoolSettingCell>("core.overlay.always-show", "Always Show Overlay", "");

    // Audio
    this->addHeader("core.audio", "Audio", m_voiceTab);
    this->addSetting<BoolSettingCell>("core.audio.voice-chat-enabled", "Voice Chat", "");
    this->addSetting<FloatSettingCell>("core.audio.playback-volume", "Voice Volume", "");
    this->addSetting(ButtonSettingCell::create("Audio Device", "", "Set", [] {
        AudioDeviceSetupPopup::create()->show();
    }, CELL_SIZE));
    this->addSetting<BoolSettingCell>("core.audio.voice-proximity", "Voice Proximity (Plat)", "");
    this->addSetting<BoolSettingCell>("core.audio.classic-proximity", "Voice Proximity (Classic)", "");
    this->addSetting<BoolSettingCell>("core.audio.deafen-notification", "Deafen Notification", "");
    this->addSetting<BoolSettingCell>("core.audio.only-friends", "Friends Only Voice", "");
    this->addSetting<BoolSettingCell>("core.audio.voice-loopback", "Voice Loopback", "");
    this->addSetting<IntSliderSettingCell>("core.audio.buffer-size", "Audio Buffer Size", "");

    // Menus
    this->addHeader("core.ui", "Menus", m_menusTab);
    this->addSetting<BoolSettingCell>("core.streamer-mode", "Streamer Mode", "");
    this->addSetting<BoolSettingCell>("core.ui.increase-level-list", "Increase Level List", "");
    this->addSetting<BoolSettingCell>("core.ui.compressed-player-count", "Simple Player Count", "");

    for (auto tab : m_tabs) {
        tab->updateLayout();
    }
}

void SettingsLayer::selectTab(size_t idx) {
    GLOBED_ASSERT(idx < m_tabs.size());
    this->selectTab(m_tabs[idx]);
}

void SettingsLayer::selectTab(cue::ListNode* tab) {
    tab->setVisible(true);
    m_selectedTab = tab;

    for (auto otherTab : m_tabs) {
        if (otherTab != tab) {
            otherTab->setVisible(false);
        }
    }

    for (auto tbtn : m_tabButtons) {
        bool selected = m_tabs[tbtn->getTag()] == tab;
        tbtn->toggle(selected);
        tbtn->setZOrder(selected ? 15 : 5);

        if (selected) {
            m_tabsGradientStencil->setPosition(tbtn->m_onButton->convertToWorldSpace({}));
        }
    }

    cocos::handleTouchPriority(this, true);
}

void SettingsLayer::addHeader(CStr key, CStr text, cue::ListNode* curTab) {
    _m_curTab = curTab;
    this->addSetting<TitleSettingCell>(key, text, "");
}

void SettingsLayer::addSetting(BaseSettingCellBase* cell) {
    GLOBED_ASSERT(_m_curTab && "No current tab");
    _m_curTab->addCell(cell);
}

void SettingsLayer::refreshAll() {
    for (auto tab : m_tabs) {
        for (auto cell : tab->iter<BaseSettingCellBase>()) {
            cell->reload();
        }
    }
}

void SettingsLayer::onTabClick(CCObject* sender) {
    auto btn = static_cast<TabButton*>(sender);
    size_t idx = btn->getTag();

    this->selectTab(idx);
}

SettingsLayer* SettingsLayer::create() {
    auto ret = new SettingsLayer;
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

}
