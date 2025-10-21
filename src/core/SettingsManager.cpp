#include <globed/core/SettingsManager.hpp>
#include <globed/core/ValueManager.hpp>

#include <fmt/format.h>
#include <asp/fs.hpp>
#include <asp/iter.hpp>

using namespace geode::prelude;

namespace globed {

SettingsManager::SettingsManager() {
    this->loadSaveSlots();

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

    // Overlay
    this->registerSetting("core.overlay.enabled", true);
    this->registerSetting("core.overlay.opacity", 0.3f);
    this->registerLimits("core.overlay.opacity", 0.f, 1.f);
    this->registerSetting("core.overlay.always-show", false);
    this->registerSetting("core.overlay.position", 3);
    this->registerLimits("core.overlay.position", 0, 3); // 0: top-left, 1: top-right, 2: bottom-left, 3: bottom-right

    // Audio
    this->registerSetting("core.audio.voice-chat-enabled", true);
    this->registerSetting("core.audio.input-device", -1);
    this->registerSetting("core.audio.voice-loopback", false);

    this->registerSetting("core.audio.buffer-size", 4);
    this->registerLimits("core.audio.buffer-size", 1, 10);

    this->registerSetting("core.audio.playback-volume", 1.f);
    this->registerLimits("core.audio.playback-volume", 0.f, 2.f);

    this->registerSetting("core.audio.voice-proximity", true);
    this->registerSetting("core.audio.classic-proximity", false);
    this->registerSetting("core.audio.deafen-notification", false); // TODO: unimpl
    this->registerSetting("core.audio.only-friends", false); // TODO: unimpl

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

void SettingsManager::loadSaveSlots() {
    (void) Mod::get()->loadData();

    m_slotDir = Mod::get()->getSaveDir() / "save-slots";

    if (!asp::fs::exists(m_slotDir)) {
        auto res = asp::fs::createDirAll(m_slotDir);
        if (!res) {
            log::error("Failed to create save slots directory at {}: {}", m_slotDir, res.unwrapErr().message());
            log::warn("Will be using root of the save directory for storage");
            m_slotDir = Mod::get()->getSaveDir();
        }
    }

    for (size_t i = 0;; i++) {
        auto slotp = m_slotDir / fmt::format("{}.json", i);
        if (!asp::fs::exists(slotp)) {
            break;
        }

        auto res = geode::utils::file::readJson(slotp);
        if (!res) {
            log::error("Failed to read save slot {}: {}", i, res.unwrapErr());
            continue;
        }

        m_saveSlots.push_back(std::move(*res));
    }

    log::debug("Loaded {} save slots", m_saveSlots.size());

    auto& container = Mod::get()->getSaveContainer();
    m_activeSaveSlot = container.get("core.settingsv3.save-slot").andThen([](auto& v) {
        return v.template as<size_t>();
    }).unwrapOr(-1);

    if (m_activeSaveSlot == (size_t)-1) {
        this->migrateOldSettings();
    }

    if (m_activeSaveSlot >= m_saveSlots.size()) {
        m_activeSaveSlot = 0;
    }

    globed::setValue("core.settingsv3.save-slot", m_activeSaveSlot);
}

void SettingsManager::migrateOldSettings() {
    // TODO
}

void SettingsManager::freeze() {
    m_frozen = true;
}

void SettingsManager::commitSlotsToDisk() {
    for (size_t i = 0; i < m_saveSlots.size(); i++) {
        auto slotp = m_slotDir / fmt::format("{}.json", i);
        auto res = geode::utils::file::writeStringSafe(slotp, m_saveSlots[i].dump());

        if (!res) {
            log::error("Failed to write save slot {}: {}", i, res.unwrapErr());
        }
    }
}

std::vector<SaveSlotMeta> SettingsManager::getSaveSlots() {
    return asp::iter::from(m_saveSlots).enumerate().map([&](const auto& slot) {
        SaveSlotMeta meta;
        meta.id = slot.first;
        meta.name = slot.second.get("_saveslot-name").andThen([](auto& v) {
            return v.template as<std::string>();
        }).unwrapOr(fmt::format("Slot {}", slot.first + 1));
        meta.active = (slot.first == m_activeSaveSlot);
        return meta;
    }).collect();
}

void SettingsManager::renameSaveSlot(size_t id, std::string_view newName) {
    if (id < m_saveSlots.size()) {
        m_saveSlots[id].set("_saveslot-name", std::string(newName));
    }
}

void SettingsManager::deleteSaveSlot(size_t id) {
    if (id >= m_saveSlots.size() || id == m_activeSaveSlot) {
        return;
    }

    m_saveSlots.erase(m_saveSlots.begin() + id);

    // adjust active slot if needed
    if (m_activeSaveSlot > id) {
        m_activeSaveSlot--;

        globed::setValue("core.settingsv3.save-slot", m_activeSaveSlot);
    }
}

void SettingsManager::createSaveSlot() {
    m_saveSlots.emplace_back(matjson::Value::object());
}

void SettingsManager::switchToSaveSlot(size_t id) {
    if (id >= m_saveSlots.size() || id == m_activeSaveSlot) {
        return;
    }

    m_activeSaveSlot = id;
    globed::setValue("core.settingsv3.save-slot", m_activeSaveSlot);

    this->reloadFromSlot();
}

void SettingsManager::reloadFromSlot() {
    for (const auto& [hash, fullKey] : m_fullKeys) {
        this->reloadSetting(fullKey);
    }
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
        geode::utils::terminate(fmt::format("attempted to add the same setting twice: {}", key));
    }

    m_defaults[hash] = defaultVal;
    m_fullKeys[hash] = fullKey;

    this->reloadSetting(fullKey);
}

void SettingsManager::reloadSetting(std::string_view fullKey) {
    auto hash = this->finalKeyHash(fullKey);
    auto& defaultVal = m_defaults.at(hash);

    auto strippedKey = fullKey;
    strippedKey.remove_prefix(8); // remove "setting."

    auto tryApply = [&](const std::optional<matjson::Value>& val, std::string_view fromWhere) {
        if (!val) return false;

        if (val->type() != defaultVal.type()) {
            log::error(
                "Type mismatch for setting {} loaded from {}, expected type '{}' but got '{}'",
                strippedKey, fromWhere,
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

    // then try save slot
    // if (tryApply(ValueManager::get().getValueRaw(fullKey), "savefile")) {
    //     return;
    // }
    if (tryApply(this->findSettingInSaveSlot(fullKey), "save slot")) {
        return;
    }

    if (!tryApply(defaultVal, "default")) {
        // this should never happen!
        geode::utils::terminate(fmt::format("failed to apply default for setting {}", strippedKey));
    }
}

std::optional<matjson::Value> SettingsManager::findSettingInSaveSlot(std::string_view key) {
    if (m_activeSaveSlot >= m_saveSlots.size()) {
        return std::nullopt;
    }

    auto& slot = m_saveSlots[m_activeSaveSlot];
    return slot.get(key).copied().ok();
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

$on_mod(DataSaved) {
    SettingsManager::get().commitSlotsToDisk();
}

}