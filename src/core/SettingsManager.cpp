#include <globed/core/SettingsManager.hpp>
#include <globed/core/ValueManager.hpp>
#include <fmt/format.h>

using namespace geode::prelude;

namespace globed {

SettingsManager::SettingsManager() {
    // Preload
    this->registerSetting("core.preload.enabled", true);
    this->registerSetting("core.preload.defer", false);

    // Various
    this->registerSetting("core.autoconnect", true);

    // Keybinds
    this->registerSetting("core.keybinds.voice-chat", (int)KEY_V);
    this->registerSetting("core.keybinds.deafen", (int)KEY_B);
    this->registerSetting("core.keybinds.hide-players", (int)KEY_None);

    // UI settings
    this->registerSetting("core.ui.allow-custom-servers", false);
    this->registerSetting("core.ui.increase-level-list", false);
    this->registerSetting("core.ui.compressed-player-count", false);
    this->registerSetting("core.ui.colorblind-mode", false);

    // Player settings
    this->registerSetting("core.player.opacity", 1.0f);
    this->registerSetting("core.player.show-names", true);
    this->registerSetting("core.player.dual-name", true);
    this->registerSetting("core.player.name-opacity", 1.0f);
    this->registerSetting("core.player.force-visibility", false);
    this->registerSetting("core.player.hide-nearby-classic", false);
    this->registerSetting("core.player.hide-nearby-plat", false);
    this->registerSetting("core.player.hide-practicing", false);
    this->registerSetting("core.player.status-icons", true);
    this->registerSetting("core.player.rotate-names", true);
    this->registerSetting("core.player.death-effects", true);
    this->registerSetting("core.player.default-death-effects", false);

    // Level UI
    this->registerSetting("core.level.progress-indicators", true);
    this->registerSetting("core.level.progress-indicators-plat", true);
    this->registerSetting("core.level.progress-opacity", 1.0f);
    this->registerSetting("core.level.voice-overlay", true);
    this->registerSetting("core.level.force-progressbar", false);
    this->registerSetting("core.level.self-status-icons", true);
    this->registerSetting("core.level.self-name", false);

    // Audio
    this->registerSetting("core.audio.voice-chat-enabled", true);
    this->registerSetting("core.audio.input-device", -1);
    this->registerSetting("core.audio.voice-loopback", false);

    this->registerSetting("core.audio.buffer-size", 4);
    this->registerLimits("core.audio.buffer-size", 1, 10);

    this->registerSetting("core.audio.playback-volume", 1.f);
    this->registerLimits("core.audio.playback-volume", 0.f, 2.f);

    // Mod settings
    this->registerSetting("core.mod.remember-password", false);

    // User settings (custom UI)
    this->registerSetting("core.user.hide-in-levels", false);
    this->registerSetting("core.user.hide-in-menus", false);
    this->registerSetting("core.user.hide-roles", false);

    // Developer settings
    this->registerSetting("core.dev.packet-loss-sim", 0.0f);
    this->registerSetting("core.dev.net-debug-logs", false);
    this->registerSetting("core.dev.fake-data", false);
}

void SettingsManager::freeze() {
    m_frozen = true;
}

static std::optional<matjson::Value> findOverride(std::string_view key) {
    static auto overrideMap = []{
        std::unordered_map<std::string, matjson::Value> map;
        auto names = Loader::get()->getLaunchArgumentNames();

        for (auto& name : names) {
            if (name.starts_with("globed/")) {
                auto val = *Loader::get()->getLaunchArgument(name);

                // if there's no value, assume it is a bool flag
                if (val.empty()) {
                    map[name.substr(7)] = true;
                    continue;
                }

                auto res = matjson::parse(val);

                if (res) {
                    map[name.substr(7)] = *res;
                } else {
                    log::error("Failed to parse launch argument override '{}': {}", name, res.unwrapErr());
                }
            }
        }

        // Print all the overrides
        for (const auto& [k, v] : map) {
            log::debug("Launch argument override: {} = {}", k, v.dump(matjson::NO_INDENTATION));
        }

        return map;
    }();

    auto it = overrideMap.find(std::string(key));
    if (it != overrideMap.end()) {
        auto& value = it->second;
        return value;
    }

    return std::nullopt;
}

static std::string_view matjsonTypeToStr(matjson::Type t) {
    switch (t) {
        case matjson::Type::Null: return "null";
        case matjson::Type::Bool: return "bool";
        case matjson::Type::Number: return "number";
        case matjson::Type::String: return "string";
        case matjson::Type::Object: return "object";
        case matjson::Type::Array: return "array";
        default: return "unknown";
    }
}

void SettingsManager::registerSetting(
    std::string_view key,
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

    auto tryApply = [&](const std::optional<matjson::Value>& val, std::string_view fromWhere) {
        if (!val) return false;

        if (val->type() != defaultVal.type()) {
            log::error(
                "Type mismatch for setting {} loaded from {}, expected type '{}' but got '{}'",
                key, fromWhere,
                matjsonTypeToStr(defaultVal.type()), matjsonTypeToStr(val->type())
            );

            return false;
        }

        m_settings[hash] = *val;
        return true;
    };

    // try overrides first
    if (tryApply(findOverride(fullKey), "launch args")) {
        return;
    }

    if (tryApply(ValueManager::get().getValueRaw(fullKey), "savefile")) {
        return;
    }

    if (!tryApply(defaultVal, "default")) {
        // this should never happen!
        throw std::runtime_error(fmt::format("failed to apply default value for setting {}", key));
    }
}

void SettingsManager::registerValidator(
    std::string_view key,
    Validator func
) {
    if (m_frozen) {
        log::error("Tried to add validator for {} after SettingsManager was frozen", key);
        return;
    }

    auto hash = this->keyHash(key);
    m_validators[hash] = std::move(func);
}

void SettingsManager::registerLimits(
    std::string_view key,
    matjson::Value min,
    matjson::Value max
) {
    if (m_frozen) {
        log::error("Tried to add limits for {} after SettingsManager was frozen", key);
        return;
    }

    auto hash = this->keyHash(key);
    m_limits[hash] = std::make_pair(std::move(min), std::move(max));
}

std::optional<std::pair<matjson::Value, matjson::Value>> SettingsManager::getLimits(std::string_view key) {
    auto hash = this->keyHash(key);
    auto it = m_limits.find(hash);

    return it == m_limits.end() ? std::nullopt : std::optional(it->second);
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