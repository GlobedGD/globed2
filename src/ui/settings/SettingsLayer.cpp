#include "SettingsLayer.hpp"
#include "BoolSettingCell.hpp"
#include "FloatSettingCell.hpp"
#include "TitleSettingCell.hpp"
#include "ButtonSettingCell.hpp"
#include "DiscordLinkPopup.hpp"
#include "KeybindsPopup.hpp"
#include <globed/core/PopupManager.hpp>
#include <core/net/NetworkManagerImpl.hpp>

#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace globed {

bool SettingsLayer::init() {
    BaseLayer::init(true);

    auto winSize = CCDirector::get()->getWinSize();

    m_list = Build(cue::ListNode::create({356.f, 220.f}, {191, 114, 62, 255}))
        .pos(winSize / 2.f)
        .parent(this);
    m_list->setAutoUpdate(false);

    this->addSettings();

    return true;
}

void SettingsLayer::addSettings() {
    // TODO: descriptions
    // Globed
    this->addSetting<BoolSettingCell>("core.autoconnect", "Autoconnect", "");
    this->addSetting<BoolSettingCell>("core.ui.increase-level-list", "Increase Level List", "");
    this->addSetting<BoolSettingCell>("core.ui.compressed-player-count", "Simple Player Count", "");
    this->addSetting(ButtonSettingCell::create("Discord Linking", "", "Link", [this] {
        if (!NetworkManagerImpl::get().isConnected()) {
            globed::alert("Error", "Cannot do this while not connected to a server.");
            return;
        }

        DiscordLinkPopup::create()->show();
    }, CELL_SIZE));
    this->addSetting(ButtonSettingCell::create("Keybinds", "", "Edit", [this] {
        KeybindsPopup::create()->show();
    }, CELL_SIZE));

    // Player settings
    this->addHeader("core.player", "Players");
    this->addSetting<FloatSettingCell>("core.player.opacity", "Player Opacity", "");
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
    this->addHeader("core.level", "Level UI");
    this->addSetting<BoolSettingCell>("core.level.progress-indicators", "Progress Icons (Classic)", "");
    this->addSetting<BoolSettingCell>("core.level.progress-indicators-plat", "Progress Icons (Plat)", "");
    this->addSetting<FloatSettingCell>("core.level.progress-opacity", "Progress Opacity", "");
    this->addSetting<BoolSettingCell>("core.level.voice-overlay", "Voice Chat Overlay", "");
    this->addSetting<BoolSettingCell>("core.level.force-progressbar", "Force progressbar", "");
    this->addSetting<BoolSettingCell>("core.level.self-status-icons", "Show Own Status Icons", "");
    this->addSetting<BoolSettingCell>("core.level.self-name", "Show Own Name", "");

    // Overlay
    this->addHeader("core.overlay", "Ping overlay");
    this->addSetting<BoolSettingCell>("core.overlay.enabled", "Enable Overlay", "");
    this->addSetting<FloatSettingCell>("core.overlay.opacity", "Overlay Opacity", "");
    // this->addSetting<IntSettingCell>("core.overlay.position", "Overlay Position", ""); // TODO (release)
    this->addSetting<BoolSettingCell>("core.overlay.always-show", "Always Show Overlay", "");

    // Audio
    this->addHeader("core.audio", "Audio");
    this->addSetting<BoolSettingCell>("core.audio.voice-chat-enabled", "Voice Chat", "");
    this->addSetting<FloatSettingCell>("core.audio.playback-volume", "Voice Volume", "");
    this->addSetting(ButtonSettingCell::create("Audio Device", "", "Set", [this] {
        // TODO: popup with choosing audio device
    }, CELL_SIZE));
    this->addSetting<BoolSettingCell>("core.audio.voice-loopback", "Voice Loopback", "");
    // TODO: buffer size setting? might need a new IntSettingCell

    // Preload
    this->addHeader("core.player", "Preloading");
    this->addSetting<BoolSettingCell>("core.preload.enabled", "Preload Assets", "");
    this->addSetting<BoolSettingCell>("core.preload.defer", "Defer Preloading", "");

    // Advanced settings
    this->addHeader("core.dev", "Advanced");
    this->addSetting<BoolSettingCell>("core.ui.allow-custom-servers", "Allow Custom Servers", "");

#ifdef GLOBED_DEBUG
    bool showDebug = true;
#else
    bool showDebug = globed::value<bool>("core.dev.enable-dev-settings").value_or(false);
#endif

    if (showDebug) {
        this->addSetting<FloatSettingCell>("core.dev.packet-loss-sim", "Packet Loss Simulation", "");
        this->addSetting<BoolSettingCell>("core.dev.net-debug-logs", "Network Debug Logs", "");
        this->addSetting<BoolSettingCell>("core.dev.fake-data", "Use Fake Data", "");
    }

    m_list->updateLayout();
}

void SettingsLayer::addHeader(CStr key, CStr text) {
    this->addSetting<TitleSettingCell>(key, text, "");
}

void SettingsLayer::addSetting(CCNode* cell) {
    m_list->addCell(cell);
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
