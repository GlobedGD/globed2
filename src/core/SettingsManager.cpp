#include <globed/core/SettingsManager.hpp>
#include <globed/core/ValueManager.hpp>

#include <Geode/utils/function.hpp>
#include <Geode/utils/StringMap.hpp>
#include <fmt/format.h>
#include <asp/fs.hpp>
#include <asp/iter.hpp>

using namespace geode::prelude;

namespace globed {

// Note for future self: launch arg overides are in this format:
// core.xxx -> --geode:globed/setting.core.xxx
SettingsManager::SettingsManager() {
    this->loadSaveSlots();

    // Preload
    this->registerSetting(Setting::Preload::Enabled, true);
    this->registerSetting(Setting::Preload::Defer, false);
    this->registerSetting(Setting::Preload::ForcePreload, false);
    // hidden preload settings
    this->registerSetting(Setting::Preload::BatchSize, 0);
    this->registerSetting(Setting::Preload::UsePbos, true);
    this->registerSetting(Setting::Preload::UseDirectDecode, true);

    // Various
    this->registerSetting(Setting::General::Autoconnect, true);
    this->registerSetting(Setting::General::StreamerMode, false);
    this->registerSetting(Setting::General::InvitesFrom, (int)InvitesFrom::Friends);

    // Editor related
    this->registerSetting(Setting::Editor::Enabled, true);

    // Keybinds
    this->registerSetting(Setting::Keybinds::VoiceChat, (int)KEY_V);
    this->registerSetting(Setting::Keybinds::Deafen, (int)KEY_B);
    this->registerSetting(Setting::Keybinds::HidePlayers, (int)KEY_None);
    this->registerSetting(Setting::Keybinds::Emote0, (int)KEY_None);
    this->registerSetting(Setting::Keybinds::Emote1, (int)KEY_None);
    this->registerSetting(Setting::Keybinds::Emote2, (int)KEY_None);
    this->registerSetting(Setting::Keybinds::Emote3, (int)KEY_None);
    this->registerSetting(Setting::Keybinds::Emote4, (int)KEY_None);
    this->registerSetting(Setting::Keybinds::Emote5, (int)KEY_None);
    this->registerSetting(Setting::Keybinds::Emote6, (int)KEY_None);
    this->registerSetting(Setting::Keybinds::Emote7, (int)KEY_None);

    // UI settings
    this->registerSetting(Setting::Ui::AllowCustomServers, false);
    this->registerSetting(Setting::Ui::IncreaseLevelList, false);
    this->registerSetting(Setting::Ui::CompressedPlayerCount, true);
    this->registerSetting(Setting::Ui::ColorblindMode, false);
    this->registerSetting(Setting::Ui::DisableNotices, false);

    // Player settings
    this->registerSetting(Setting::Player::Opacity, 1.0f);
    this->registerLimits(Setting::Player::Opacity, 0.f, 1.f);
    this->registerSetting(Setting::Player::QuickChatEnabled, true);
    this->registerSetting(Setting::Player::QuickChatSfx, true);
    this->registerSetting(Setting::Player::QuickChatSfxVolume, 0.5f);
    this->registerSetting(Setting::Player::EmoteOpacity, 1.0f);
    this->registerLimits(Setting::Player::EmoteOpacity, 0.f, 1.f);
    this->registerSetting(Setting::Player::ShowNames, true);
    this->registerSetting(Setting::Player::DualName, true);
    this->registerSetting(Setting::Player::NameOpacity, 1.0f);
    this->registerSetting(Setting::Player::ForceVisibility, false);
    this->registerSetting(Setting::Player::HideNearbyClassic, false);
    this->registerSetting(Setting::Player::HideNearbyPlat, false);
    this->registerSetting(Setting::Player::HidePracticing, false);
    this->registerSetting(Setting::Player::StatusIcons, true);
    this->registerSetting(Setting::Player::RotateNames, true);
    this->registerSetting(Setting::Player::DeathEffects, true);
    this->registerSetting(Setting::Player::DefaultDeathEffects, false);
    // invisible settings
    this->registerSetting(Setting::Player::BlacklistedPlayers, matjson::Value::array());
    this->registerSetting(Setting::Player::WhitelistedPlayers, matjson::Value::array());
    this->registerSetting(Setting::Player::HiddenPlayers, matjson::Value::array());
    this->refreshPlayerLists();

    // Level UI
    this->registerSetting(Setting::Level::ProgressIndicators, true);
    this->registerSetting(Setting::Level::ProgressIndicatorsPlat, true);
    this->registerSetting(Setting::Level::ProgressOpacity, 1.0f);
    this->registerSetting(Setting::Level::ForceProgressbar, false);
    this->registerSetting(Setting::Level::SelfStatusIcons, true);
    this->registerSetting(Setting::Level::SelfName, false);
    this->registerSetting(Setting::Level::FixProgressBar, true);
    this->registerSetting(Setting::Level::VoiceOverlay, true);
    this->registerSetting(Setting::Level::VoiceOverlayThreshold, 0.05f);
    this->registerSetting(Setting::Level::VoiceOverlayPosition, 3);
    this->registerLimits(Setting::Level::VoiceOverlayPosition, 0, 3); // 0: top-left, 1: top-right, 2: bottom-left, 3: bottom-right
    this->registerSetting(Setting::Level::VoiceOverlayPadY, 26.f);
    this->registerLimits(Setting::Level::VoiceOverlayPadY, 0.f, 100.f);

    // Overlay
    this->registerSetting(Setting::Overlay::Enabled, true);
    this->registerSetting(Setting::Overlay::Opacity, 0.35f);
    this->registerLimits(Setting::Overlay::Opacity, 0.f, 1.f);
    this->registerSetting(Setting::Overlay::AlwaysShow, false);
    this->registerSetting(Setting::Overlay::Position, 3);
    this->registerLimits(Setting::Overlay::Position, 0, 3); // 0: top-left, 1: top-right, 2: bottom-left, 3: bottom-right

    // Audio
    this->registerSetting(Setting::Audio::VoiceChatEnabled, true);
    this->registerSetting(Setting::Audio::InputDevice, -1); // deprecated
    this->registerSetting(Setting::Audio::InputDeviceGuid, "");
    this->registerSetting(Setting::Audio::VoiceLoopback, false);
    this->registerSetting(Setting::Audio::OverlayingOverlay, false);

    this->registerSetting(Setting::Audio::BufferSize, 4);
    this->registerLimits(Setting::Audio::BufferSize, 1, 10);

    this->registerSetting(Setting::Audio::PlaybackVolume, 1.f);
    this->registerLimits(Setting::Audio::PlaybackVolume, 0.f, 2.f);

    this->registerSetting(Setting::Audio::VoiceProximity, true);
    this->registerSetting(Setting::Audio::ClassicProximity, false);
    this->registerSetting(Setting::Audio::DeafenNotification, false);
    this->registerSetting(Setting::Audio::OnlyFriends, false); // TODO make this work server side?

    // Mod settings
    this->registerSetting(Setting::ModSettings::RememberPassword, false);

    // User settings (custom UI)
    this->registerSetting(Setting::User::AllowUserSettings, false);
    this->registerSetting(Setting::User::HideInLevels, false);
    this->registerSetting(Setting::User::HideInMenus, false);
    this->registerSetting(Setting::User::HideRoles, false);

    // Developer settings
    this->registerSetting(Setting::Dev::PacketLossSim, 0.0f);
    this->registerSetting(Setting::Dev::NetDebugLogs, false);
    this->registerSetting(Setting::Dev::NetStatDump, false);
    this->registerSetting(Setting::Dev::NetPreferProto, (int)PreferConnection::Auto);
    this->registerSetting(Setting::Dev::NetUseIpv4, false);
    this->registerSetting(Setting::Dev::NetDontOverrideDns, false);
    this->registerSetting(Setting::Dev::FakeData, false);
    this->registerSetting(Setting::Dev::CertVerification, true);
    this->registerSetting(Setting::Dev::GhostFollower, false);
    this->registerSetting(Setting::Dev::ProfileFrameTime, false);
}

void SettingsManager::loadSaveSlots() {
    (void) Mod::get()->loadData();

    m_slotDir = Mod::get()->getSaveDir() / "save-slots-v2";
    auto oldDir = Mod::get()->getSaveDir() / "saveslots";

    bool createdNewDir = false;
    if (!asp::fs::exists(m_slotDir)) {
        auto res = asp::fs::createDirAll(m_slotDir);
        if (!res) {
            log::error("Failed to create save slots directory at {}: {}", m_slotDir, res.unwrapErr().message());
            log::warn("Will be using root of the save directory for storage");
            m_slotDir = Mod::get()->getSaveDir();
        } else {
            createdNewDir = true;
        }
    }

    // only run the migration on first run, if the new slot dir already exists then ignore
    if (asp::fs::exists(oldDir) && createdNewDir) {
        this->migrateOldSettings();
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

        auto obj = std::move(res).unwrap();
        if (!obj.isObject()) {
            log::error("Save slot {} is not a JSON object, skipping", i);
            continue;
        }

        m_saveSlots.push_back(std::move(obj));
    }

    log::debug("Loaded {} save slots", m_saveSlots.size());

    auto& container = Mod::get()->getSaveContainer();
    m_activeSaveSlot = container.get("core.settingsv3.save-slot").andThen([](auto& v) {
        return v.template as<size_t>();
    }).unwrapOr(-1);

    if (m_activeSaveSlot >= m_saveSlots.size()) {
        m_activeSaveSlot = 0;
    }

    if (m_saveSlots.empty()) {
        this->createSaveSlot();
    }

    globed::setValue("core.settingsv3.save-slot", m_activeSaveSlot);
}

static std::optional<matjson::Value> migrateSlot(const matjson::Value& slot) {
    using MapperFn = geode::FunctionRef<matjson::Value(const matjson::Value&)>;

    matjson::Value out;

    if (auto name = slot.get("_saveslot-name").copied().ok()) {
        out.set("_saveslot-name", std::move(*name));
    }

    auto migMapper = [&](std::string_view bCat, std::string_view bKey, std::string_view after, MapperFn mapper) -> bool {
        auto fullKey = fmt::format("{}{}", bCat, bKey);
        if (auto val = slot.get(fullKey).copied().ok()) {
            log::info("Migrating setting from old format: {} -> {}", fullKey, after);
            out.set(fmt::format("setting.{}", after), mapper(std::move(*val)));
            return true;
        }

        return false;
    };

    auto migDirect = [&](std::string_view bCat, std::string_view bKey, std::string_view after) -> bool {
        return migMapper(bCat, bKey, after, [](auto&& v) { return v; });
    };

    auto migInvertBool = [&](std::string_view bCat, std::string_view bKey, std::string_view after) {
        return migMapper(bCat, bKey, after, [](auto&& v) {
            if (!v.isBool()) {
                return v;
            }

            return matjson::Value(!v.asBool().unwrap());
        });
    };

    // Globed
    migDirect("globed", "autoconnect", "core.autoconnect");
    migDirect("globed", "preloadAssets", "core.preload.enabled");
    migDirect("globed", "deferPreloadAssets", "core.preload.defer");
    // migDirect("globed", "invitesFrom", "");
    // migDirect("globed", "editorSupport", "");
    migDirect("globed", "increaseLevelList", "core.ui.increase-level-list");
    // migDirect("globed", "forceTcp", "");
    // migDirect("globed", "dnsOverHttps", "");
    // migDirect("globed", "showRelays", "");
    migDirect("globed", "compressedPlayerCount", "core.ui.compressed-player-count");
    // migDirect("globed", "useDiscordRPC", "");
    // migDirect("globed", "changelogPopups", "");
    // migDirect("globed", "editorChanges", "");
    migDirect("globed", "isInvisible", "core.user.hide-in-menus");
    migDirect("globed", "hideInGame", "core.user.hide-in-levels");
    migDirect("globed", "hideRoles", "core.use.hide-roles");

    // Overlay
    migDirect("overlay", "enabled", "core.overlay.enabled");
    migDirect("overlay", "opacity", "core.overlay.opacity");
    migInvertBool("overlay", "hideConditionally", "core.overlay.always-show");
    migDirect("overlay", "position", "core.overlay.position");

    // Communication
    migDirect("communication", "voiceEnabled", "core.audio.voice-chat-enabled");
    migDirect("communication", "voiceProximity", "core.audio.voice-proximity");
    migDirect("communication", "classicProximity", "core.audio-classic-proximity");
    migDirect("communication", "voiceVolume", "core.audio.playback-volume");
    migDirect("communication", "onlyFriends", "core.audio.only-friends");
    migMapper("communication", "lowerAudioLatency", "core.audio.buffer-size", [](auto&& v) {
        if (!v.isBool()) {
            return matjson::Value(4);
        }

        return matjson::Value(v.asBool().unwrap() ? 4 : 8);
    });
    // migDirect("communication", "tcpAudio", "");
    migDirect("communication", "audioDevice", "core.audio.input-device");
    migDirect("communication", "deafenNotification", "core.audio.deafen-notification");
    migDirect("communication", "voiceLoopback", "core.audio.voice-loopback");

    // LevelUI
    migDirect("levelui", "progressIndicators", "core.level.progress-indicators");
    migDirect("levelui", "progressPointers", "core.level.progress-indicators-plat");
    migDirect("levelui", "progressOpacity", "core.level.progress-opacity");
    migDirect("levelui", "voiceOverlay", "core.level.voice-overlay");
    migDirect("levelui", "forceProgressBar", "core.level.force-progress-bar");

    // Players
    migDirect("players", "playerOpacity", "core.player.opacity");
    migDirect("players", "showNames", "core.player.show-names");
    migDirect("players", "dualName", "core.player.dual-name");
    migDirect("players", "nameOpacity", "core.player.name-opacity");
    migDirect("players", "statusIcons", "core.player.status-icons");
    migDirect("players", "deathEffects", "core.player.death-effects");
    migDirect("players", "defaultDeathEffect", "core.player.default-death-effects");
    // migDirect("players", "hideNearby", "core.player.hide-nearby-classic"); // not migrated? because its split
    migDirect("players", "forceVisibility", "core.player.force-visibility");
    migDirect("players", "ownName", "core.level.self-name");
    migDirect("players", "rotateNames", "core.player.rotate-names");
    migDirect("players", "hidePracticePlayers", "core.player.hide-practicing");

    // Keys
    migDirect("keys", "voiceChatKey", "core.keybinds.voice-chat");
    migDirect("keys", "voiceDeafenKey", "core.keybinds.deafen");
    migDirect("keys", "hidePlayersKey", "core.keybinds.hide-players");

    // TODO: migrate some flags

    return out;
}

void SettingsManager::migrateOldSettings() {
    log::info("Migrating legacy save slots");

    auto oldDir = Mod::get()->getSaveDir() / "saveslots";

    for (size_t i = 0;; i++) {
        auto slotp = oldDir / fmt::format("saveslot-{}.json", i);
        auto newp = m_slotDir / fmt::format("{}.json", i);

        if (!asp::fs::exists(slotp)) {
            break;
        }

        log::info("Migrating: {} -> {}", slotp, newp);

        auto res = geode::utils::file::readJson(slotp);
        if (!res) {
            log::error("Failed to read old save slot {}: {}", i, res.unwrapErr());
            continue;
        }

        auto obj = std::move(res).unwrap();
        if (!obj.isObject()) {
            log::error("Old save slot {} is not a JSON object, skipping", i);
            continue;
        }

        auto migrated = migrateSlot(obj);
        if (!migrated) {
            log::warn("Failed to migrate old save slot {}", i);
            continue;
        }

        // if (auto err = asp::fs::remove(slotp).err()) {
        //     log::error("Failed to delete old save slot {}: {}", i, err->message());
        //     continue;
        // }

        if (auto err = geode::utils::file::writeStringSafe(newp, migrated->dump()).err()) {
            log::error("Failed to write migrated save slot {}: {}", i, *err);
            continue;
        }
    }

    // (void) asp::fs::removeDir(oldDir);
    // add a small readme file that says this folder isnt used anymore
    auto rmpath = oldDir / "README.txt";
    auto content = "This folder was used by Globed v1.x.x and is no longer used since Globed v2.\n\n"
        "You can safely delete this folder if you want, it is kept only so that users can switch between old and new releases without losing their settings,"
        " while still automatically migrating old settings to the new format if necessary.";

    (void) geode::utils::file::writeStringSafe(rmpath, content);
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
    m_saveSlots.back().set("_saveslot-name", fmt::format("Slot {}", m_saveSlots.size()));
}

void SettingsManager::switchToSaveSlot(size_t id) {
    if (id >= m_saveSlots.size() || id == m_activeSaveSlot) {
        return;
    }

    m_activeSaveSlot = id;
    globed::setValue("core.settingsv3.save-slot", m_activeSaveSlot);

    this->reloadFromSlot();
}

bool SettingsManager::isPlayerBlacklisted(int id) {
    return m_blacklisted.contains(id);
}

bool SettingsManager::isPlayerWhitelisted(int id) {
    return m_whitelisted.contains(id);
}

bool SettingsManager::isPlayerHidden(int id) {
    return m_hidden.contains(id);
}

void SettingsManager::refreshPlayerLists() {
    auto bl = this->getSettingRaw<std::vector<int>>(this->keyHash("core.player.blacklisted-players"));
    auto wl = this->getSettingRaw<std::vector<int>>(this->keyHash("core.player.whitelisted-players"));
    auto hl = this->getSettingRaw<std::vector<int>>(this->keyHash("core.player.hidden-players"));

    m_whitelisted = asp::iter::from(wl).collect<std::unordered_set<int>>();
    m_blacklisted = asp::iter::from(bl).collect<std::unordered_set<int>>();
    m_hidden = asp::iter::from(hl).collect<std::unordered_set<int>>();
}

void SettingsManager::commitPlayerLists() {
    std::vector<int> bl{asp::iter::from(m_blacklisted).collect()};
    std::vector<int> wl{asp::iter::from(m_whitelisted).collect()};
    std::vector<int> hl{asp::iter::from(m_hidden).collect()};

    this->setSettingRaw(this->keyHash("core.player.blacklisted-players"), std::move(bl));
    this->setSettingRaw(this->keyHash("core.player.whitelisted-players"), std::move(wl));
    this->setSettingRaw(this->keyHash("core.player.hidden-players"), std::move(hl));
}

void SettingsManager::blacklistPlayer(int id) {
    m_blacklisted.insert(id);
    m_whitelisted.erase(id);
}

void SettingsManager::whitelistPlayer(int id) {
    m_whitelisted.insert(id);
    m_blacklisted.erase(id);
}

void SettingsManager::setPlayerHidden(int id, bool hidden) {
    if (hidden) {
        m_hidden.insert(id);
    } else {
        m_hidden.erase(id);
    }
}

void SettingsManager::reloadFromSlot() {
    for (const auto& [hash, fullKey] : m_fullKeys) {
        this->reloadSetting(fullKey);
    }
}

static std::optional<matjson::Value> findOverride(std::string_view key) {
    static auto overrideMap = []{
        geode::utils::StringMap<matjson::Value> map;
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

    auto it = overrideMap.find(key);
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

void SettingsManager::reset() {
    GLOBED_ASSERT(m_activeSaveSlot < m_saveSlots.size());
    auto& slot = m_saveSlots[m_activeSaveSlot];

    // preserve just the name
    auto name = slot.get("_saveslot-name").copied().ok();

    slot.clear();

    if (name) {
        slot.set("_saveslot-name", std::move(*name));
    }

    this->reloadFromSlot();
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

bool SettingsManager::listenForChangesRaw(
    std::string_view key,
    geode::Function<void(const matjson::Value&)> callback
) {
    auto hash = this->keyHash(key);
    if (!m_settings.contains(hash)) {
        return false;
    }

    m_callbacks[hash].emplace_back(std::move(callback));
    return true;
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
    auto& sm = SettingsManager::get();
    sm.commitPlayerLists();
    sm.commitSlotsToDisk();
}

}