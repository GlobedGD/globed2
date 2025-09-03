#include "SettingsLayer.hpp"
#include "BoolSettingCell.hpp"
#include "FloatSettingCell.hpp"
#include "TitleSettingCell.hpp"

#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace globed {

bool SettingsLayer::init() {
    BaseLayer::init(true);

    auto winSize = CCDirector::get()->getWinSize();

    m_list = Build(cue::ListNode::create({356.f, 220.f}, {191, 114, 62, 255}))
        .pos(winSize / 2.f)
        .parent(this);

    this->addSettings();

    return true;
}

void SettingsLayer::addSettings() {
    m_list->setAutoUpdate(false);

    // Player settings
    this->addHeader("core.player", "Players");
    this->addSetting<FloatSettingCell>("core.player.opacity", "Player Opacity", "");
    this->addSetting<BoolSettingCell>("core.player.show-names", "Player Names", "");
    this->addSetting<BoolSettingCell>("core.player.dual-name", "Player Dual Names", "");
    this->addSetting<FloatSettingCell>("core.player.name-opacity", "Name Opacity", "");
    this->addSetting<BoolSettingCell>("core.player.force-visibility", "Force Visibility", "");
    this->addSetting<BoolSettingCell>("core.player.hide-nearby", "Hide Nearby Players", "");
    this->addSetting<BoolSettingCell>("core.player.hide-practicing", "Hide Practicing Players", "");
    this->addSetting<BoolSettingCell>("core.player.status-icons", "Show Status Icons", "");
    this->addSetting<BoolSettingCell>("core.player.rotate-names", "Rotate Names", "");
    this->addSetting<BoolSettingCell>("core.player.death-effects", "Death Effects", "");
    this->addSetting<BoolSettingCell>("core.player.default-death-effects", "Default Death Effects", "");

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

    m_list->setAutoUpdate(true);
    m_list->updateLayout();
    m_list->scrollToTop();
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
