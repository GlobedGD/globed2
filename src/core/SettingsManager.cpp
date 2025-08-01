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
        false
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