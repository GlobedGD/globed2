#include "SettingsLayer.hpp"
#include "AudioDeviceSetupPopup.hpp"
#include "cells/BoolSettingCell.hpp"
#include "cells/EnumSettingCell.hpp"
#include "cells/EmoteVolumeCell.hpp"
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
        "Voice",
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

    // Globed
    this->addHeader("core", "General", m_globedTab);
    this->addSetting<BoolSettingCell>("core.autoconnect", "Autoconnect",
        "Automatically connect to <cj>Globed</c> when launching the game, if you were connected the last time."
    );
    this->addSetting(ButtonSettingCell::create(
        "Discord Linking",
        "Link your <cb>Discord</c> account to receive <cg>roles</c> (for example <ca>Supporter</c>, <cp>Booster</c>, <cg>Moderator</c>, etc.) and unlock <cy>voice chat</c>.",
        "Link", [] {
        if (!NetworkManagerImpl::get().isConnected()) {
            globed::alert("Error", "Cannot do this while not connected to a server.");
            return;
        }

        DiscordLinkPopup::create()->show();
    }, CELL_SIZE));
    this->addSetting(ButtonSettingCell::create(
        "Keybinds",
        "Configure in-game Globed keybinds.",
        "Edit", [] {
        KeybindsPopup::create()->show();
    }, CELL_SIZE));
    this->addSetting<BoolSettingCell>("core.editor.enabled", "Show Players in Editor",
        "Hosts local (editor) levels as well, letting you see other players playing the level while you are in the editor. Note: <cy>this does not let you build levels together!</c>"
    );
    this->addSetting<EnumSettingCell>("core.invites-from", "Allow Invites From",
        "Controls who is allowed to invite you into rooms.",
        std::vector<std::pair<CStr, InvitesFrom>>{
        {"Everyone", InvitesFrom::Everyone},
        {"Friends", InvitesFrom::Friends},
        {"Nobody", InvitesFrom::Nobody},
    });
    this->addSetting<BoolSettingCell>("core.ui.disable-notices", "Disable Notices",
        "Entirely disables <cy>Notices</c>, which are a way for <cj>Globed staff</c> to communicate important information to users."
    );

    // Preload
    this->addHeader("core.player", "Preloading", m_globedTab);
    this->addSetting<BoolSettingCell>("core.preload.enabled", "Preload Assets",
        "Loads <cy>player assets</c> (icons, death effects) in advance to reduce <cr>lag spikes</c> in levels. This will use extra <cg>memory</c> and will increase <cy>load times</c> unless <cj>Defer Preloading</c> is enabled."
    );
    this->addSetting<BoolSettingCell>("core.preload.defer", "Defer Preloading",
        "Instead of preloading all assets on the loading screen, they are only loaded when joining a level (while connected to server)."
    );

    // Advanced settings
    this->addHeader("core.dev", "Advanced", m_globedTab);
    this->addSetting<BoolSettingCell>("core.ui.allow-custom-servers", "Allow Custom Servers",
        "Allow adding and connecting to <cy>custom Globed servers</c>, hosted by the community or yourself."
    );
    this->addSetting<BoolSettingCell>("core.dev.net-debug-logs", "Network Debug Logs",
        "Enables more detailed network logging for debugging purposes."
    );
    this->addSetting<BoolSettingCell>("core.dev.cert-verification", "SSL Verification",
        "Enables SSL certificate verification when connecting to servers. Disabling this is <cr>not recommended</c> in general, unless experiencing connection issues. This will only affect <cy>Argon authentication</c>, <cy>QUIC/WS</c> connections and <cy>DoH</c>, not TCP or UDP."
    );

    if (showDebug) {
        this->addSetting<FloatSettingCell>("core.dev.packet-loss-sim", "Packet Loss Simulation",
            "Artificially simulate outbound packet loss for testing purposes. Results depend on the transport: for <cy>TCP</c> the data is permanently lost, for <cy>UDP</c> the data may be resent if it was a reliable message, for <cy>QUIC</c> there's no loss as it will be handled by the protocol."
        );
        this->addSetting<BoolSettingCell>("core.dev.net-stat-dump", "Network Stat Dump",
            "Enables highly detailed network statistics dumping for debugging. This will write very detailed information about all inbound and outbound packets to a log file, and dump exact packet bytes. F7 can be pressed while in the Globed menu to show current connection stats. <cy>This will use a lot of memory</c>."
        );
        this->addSetting<BoolSettingCell>("core.dev.fake-data", "Use Fake Data",
            "Uses randomly generated data in some places (room list, level list) for testing purposes"
        );
        this->addSetting<BoolSettingCell>("core.dev.ghost-follower", "Ghost Follower",
            "For debugging, enables a ghost player that will follow you in levels."
        );

        this->addSetting(ButtonSettingCell::create(
            "Clear Auth Tokens",
            "Deletes all stored authentication tokens for all servers.",
            "Clear", [] {
            NetworkManagerImpl::get().clearAllUTokens();
        }, CELL_SIZE));

        this->addSetting(ButtonSettingCell::create(
            "Clear Argon Token",
            "Deletes the stored Argon authentication token.",
            "Clear", [] {
            argon::clearToken();
        }, CELL_SIZE));
    }

    // Player settings
    this->addHeader("core.player", "Players", m_playersTab);
    this->addSetting<FloatSettingCell>("core.player.opacity", "Player Opacity",
        "Sets the opacity of other players' icons in levels."
    );
    this->addSetting<BoolSettingCell>("core.player.quick-chat-enabled", "Emotes",
        "Enables the use of <cy>emotes</c> (quick chat) in levels. They can be accessed via the <cg>pause menu</c> and then bound to <cr>keybinds</c> (by default numbers 1-4)."
    );
    this->addSetting<BoolSettingCell>("core.player.quick-chat-sfx", "Emote Sound Effects",
        "Plays sound effects when you or other players use certain <cy>emotes</c> in levels."
    );
    this->addSetting<EmoteVolumeCell>("core.player.quick-chat-sfx-volume", "Emote SFX Volume",
        "Sets the volume of sound effects played when certain <cy>emotes</c> are used in levels."
    );
    this->addSetting<BoolSettingCell>("core.player.show-names", "Player Names",
        "Shows player names above their icons."
    );
    this->addSetting<BoolSettingCell>("core.player.dual-name", "Player Dual Names",
        "Show player names on secondary icons as well."
    );
    this->addSetting<FloatSettingCell>("core.player.name-opacity", "Name Opacity",
        "Sets the opacity of other players' names in levels."
    );
    this->addSetting<BoolSettingCell>("core.player.force-visibility", "Force Visibility",
        "Forces other players to always be visible in levels, even when hidden by e.g. hide triggers."
    );
    this->addSetting<BoolSettingCell>("core.player.hide-nearby-classic", "Hide Nearby (Classic)",
        "Fade nearby players in <cg>classic levels</c> to reduce visual clutter with a lot of people nearby."
    );
    this->addSetting<BoolSettingCell>("core.player.hide-nearby-plat", "Hide Nearby (Plat)",
        "Fade nearby players in <cg>platformer levels</c> to reduce visual clutter with a lot of people nearby."
    );
    this->addSetting<BoolSettingCell>("core.player.hide-practicing", "Hide Practicing Players",
        "Hide players that are in practice mode."
    );
    this->addSetting<BoolSettingCell>("core.player.status-icons", "Show Status Icons",
        "Show icons indicating player states, such as paused, practicing, voice chatting, editor."
    );
    this->addSetting<BoolSettingCell>("core.player.rotate-names", "Rotate Names",
        "Rotates player names together with the camera to keep them aligned."
    );
    this->addSetting<BoolSettingCell>("core.player.death-effects", "Death Effects",
        "Plays other players' death effects when they die in levels."
    );
    this->addSetting<BoolSettingCell>("core.player.default-death-effects", "Default Death Effects",
        "Replaces all player death effects with the default explosion effect."
    );
    this->addSetting<BoolSettingCell>("core.level.self-status-icons", "Show Own Status Icons",
        "Show your own status icons. (paused, speaking, etc.)"
    );
    this->addSetting<BoolSettingCell>("core.level.self-name", "Show Own Name",
        "Show your own name in levels."
    );

    // Level UI
    this->addHeader("core.level", "Level UI", m_levelUiTab);
    this->addSetting<BoolSettingCell>("core.level.progress-indicators", "Progress Markers (Classic)",
        "Show icons under the progress bar indicating other players' <cy>progress</c> in <cg>classic levels</c>."
    );
    this->addSetting<BoolSettingCell>("core.level.progress-indicators-plat", "Direction Markers (Plat)",
        "Show icons near edges of the screen indicating where other players are located (if offscreen) in <cg>platformer levels</c>."
    );
    this->addSetting<FloatSettingCell>("core.level.progress-opacity", "Progress Opacity",
        "Adjusts the opacity of progress/direction markers of other players."
    );
    this->addSetting<BoolSettingCell>("core.level.force-progressbar", "Force Progress Bar",
        "Forces the <cg>progress bar</c> to be shown in classic levels when connected to Globed, even if the user has it disabled."
    );

    // Voice overlay
    this->addHeader("core.level.voice", "Voice Chat Overlay", m_levelUiTab);
    this->addSetting<BoolSettingCell>("core.level.voice-overlay", "Enabled",
        "Show an overlay indicating which players are currently <cg>speaking</c>."
    );
    this->addSetting<FloatSettingCell>("core.level.voice-overlay-threshold", "Volume Threshold",
        "Adjust how loud players have to be speaking to be shown on the voice overlay."
    );

    // Overlay
    this->addHeader("core.overlay", "Ping overlay", m_levelUiTab);
    this->addSetting<BoolSettingCell>("core.overlay.enabled", "Enable Overlay",
        "Show a small text overlay displaying your current <cy>ping</c> to the server."
    );
    this->addSetting<FloatSettingCell>("core.overlay.opacity", "Overlay Opacity",
        "Set the opacity of the ping overlay."
    );
    this->addSetting<IntCornerSettingCell>("core.overlay.position", "Overlay Position",
        "Choose where the ping overlay appears on the screen."
    );
    this->addSetting<BoolSettingCell>("core.overlay.always-show", "Always Show Overlay",
        "Show the <cy>ping overlay</c> even when not connected or in unsupported levels, replacing the ping with a text like <cr>'Not connected'</c> or <cr>'N/A'</c>."
    );

    // Audio
    this->addHeader("core.audio", "Audio", m_voiceTab);
    this->addSetting<BoolSettingCell>("core.audio.voice-chat-enabled", "Voice Chat",
        "Enable in-game voice chat (default keybind is V). Note: <cy>this is currently only supported on Windows, and requires you to link your </c><cb>Discord</c><cy>account</c>."
    );
    this->addSetting<FloatSettingCell>("core.audio.playback-volume", "Voice Volume",
        "Adjust the global voice chat volume."
    );
    this->addSetting(ButtonSettingCell::create(
        "Audio Device",
        "Sets the used input device for voice chat.",
        "Set", [] {
#ifdef GLOBED_VOICE_CAN_TALK
        AudioDeviceSetupPopup::create()->show();
#else
        globed::alert(
            "Error",
#ifdef GEODE_IS_MACOS
            "Microphone support is currently <cr>not available</c> on this platform, and is unlikely to be added in the future due to macOS restrictions."
#else
            "Microphone support is currently <cr>not available</c> on this platform."
#endif
        );
#endif
    }, CELL_SIZE));
    this->addSetting<BoolSettingCell>("core.audio.voice-proximity", "Voice Proximity (Plat)",
        "Enables proximity voice chat in <cg>platformer</c> levels, meaning players get quieter the further away they are from you."
    );
    this->addSetting<BoolSettingCell>("core.audio.classic-proximity", "Voice Proximity (Classic)",
        "Enables proximity voice chat in <cg>classic</c> levels, meaning players get quieter the further away they are from you."
    );
    this->addSetting<BoolSettingCell>("core.audio.deafen-notification", "Deafen Notification",
        "Show a notification when you deafen or undeafen."
    );
    this->addSetting<BoolSettingCell>("core.audio.only-friends", "Friends Only Voice",
        "Only hear voice chat from your <cg>friends</c>."
    );
    this->addSetting<BoolSettingCell>("core.audio.voice-loopback", "Voice Loopback",
        "Hear your own voice while speaking."
    );
    this->addSetting<BoolSettingCell>("core.audio.overlaying-overlay", "Overlay On Top",
        "Render the voice overlay above other UI elements, allowing it to be used in the <cg>pause menu</c>."
    );
    this->addSetting<IntSliderSettingCell>("core.audio.buffer-size", "Audio Buffer Size",
        "Adjusts the audio buffer size (in 60ms frames). Lower sizes reduce audio latency but may decrease stability."
    );

    // Menus
    this->addHeader("core.ui", "Menus", m_menusTab);
    this->addSetting<BoolSettingCell>("core.streamer-mode", "Streamer Mode",
        "Hides some <cy>sensitive</c> information (room ID, password)."
    );
    this->addSetting<BoolSettingCell>("core.ui.increase-level-list", "Increase Level List",
        "Shows more levels per page in the level list."
    );
    this->addSetting<BoolSettingCell>("core.ui.compressed-player-count", "Simple Player Count",
        "Shows a simplified player count (number + icon rather than 'x players') in <cg>all places</c> instead of just <cy>Tower/Gauntlets</c>."
    );

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
