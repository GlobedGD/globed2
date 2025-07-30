#include <globed/core/SettingsManager.hpp>
#include <globed/core/ValueManager.hpp>
#include <fmt/format.h>

using namespace geode::prelude;

namespace globed {

SettingsManager::SettingsManager() {
    // Preload
    this->registerSetting(
        "core.preload.enabled",
        "Preload Assets",
        "Whether to preload assets during game loading. Helps tremendously with in-game performance, but increases loading time. May cause issues on very low-end devices.",
        true
    );

    this->registerSetting(
        "core.preload.defer",
        "Defer Preloading",
        "Skips asset preloading until you join an online level. This is useful if you don't use the mod often, but want to keep it enabled.",
        false
    );

    // Player settings
    this->registerSetting(
        "core.player.opacity",
        "Player Opacity",
        "The opacity of other players.",
        0.8f
    );

    this->registerSetting(
        "core.player.name-opacity",
        "Name Opacity",
        "The opacity of other players' names.",
        0.8f
    );

    this->registerSetting(
        "core.player.force-visibility",
        "Force Player Visibility",
        "Whether to force player visibility, even if they are not visible in the level.",
        false
    );

    this->registerSetting(
        "core.player.hide-nearby",
        "Hide Nearby Players",
        "Whether to reduce opacity of players that are nearby.",
        false
    );

    this->registerSetting(
        "core.player.hide-practicing",
        "Hide Practice Players",
        "Whether to hide players that are in practice mode.",
        false
    );

    this->registerSetting(
        "core.player.death-effects",
        "Death Effects",
        "Whether to play death effects for other players.",
        true
    );

    this->registerSetting(
        "core.player.default-death-effects",
        "Default Death Effects",
        "Whether to use the default death effects for other players.",
        true
    );

    // Developer settings
    this->registerSetting(
        "core.dev.packet-loss-sim",
        "Packet Loss Simulation",
        "Simulates packet loss for testing purposes, 0% means no loss, 100% means all packets are lost. Only works for UDP/QUIC connections, reconnect to the server to apply.",
        0.0f
    );

    this->registerSetting(
        "core.dev.net-debug-logs",
        "Network Debug Logs",
        "Enables debug logs from qunet.",
        false
    );

    this->registerSetting(
        "core.dev.fake-data",
        "Fake Data",
        "Enables fake data for testing purposes. In some places, fake data will be used instead of actual server responses.",
        false
    );
}

void SettingsManager::freeze() {
    m_frozen = true;
}

void SettingsManager::registerSetting(
    std::string_view key,
    std::string_view name,
    std::string_view description,
    matjson::Value defaultVal
) {
    if (m_frozen) {
        log::error("Tried to add setting {} after SettingsManager was frozen", key);
        return;
    }

    auto fullKey = fmt::format("setting.{}", key);
    auto hash = this->finalKeyHash(fullKey);

    if (m_defaults.contains(hash)) {
        throw std::runtime_error(fmt::format("attempted to add the same setting twice: {}", key));
    }

    m_defaults[hash] = defaultVal;
    m_fullKeys[hash] = fullKey;

    if (auto val = ValueManager::get().getValueRaw(fullKey)) {
        if (val->type() != defaultVal.type()) {
            log::error("Type mismatch for setting {}, using default value", key);
            m_settings[hash] = defaultVal;
            return;
        }

        m_settings[hash] = *val;
    } else {
        m_settings[hash] = defaultVal;
    }
}

bool SettingsManager::hasSetting(uint64_t hash) {
    return m_settings.contains(hash);
}

uint64_t SettingsManager::keyHash(std::string_view key) {
    uint64_t hash = 0xcbf29ce484222325;
    const char* pfx = "setting.";

    while (*pfx) {
        hash ^= *pfx++;
        hash *= 0x100000001b3;
    }

    for (char c : key) {
        hash ^= c;
        hash *= 0x100000001b3;
    }

    return hash;
}

uint64_t SettingsManager::finalKeyHash(std::string_view key) {
    uint64_t hash = 0xcbf29ce484222325;
    for (char c : key) {
        hash ^= c;
        hash *= 0x100000001b3;
    }
    return hash;
}

}