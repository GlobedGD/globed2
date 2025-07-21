#include <globed/core/SettingsManager.hpp>
#include <globed/core/ValueManager.hpp>
#include <fmt/format.h>

using namespace geode::prelude;

namespace globed {

SettingsManager::SettingsManager() {
    this->add(
        "core.test",
        "Test setting",
        "This is a test setting",
        42
    );

    this->freeze();
}

void SettingsManager::freeze() {
    m_frozen = true;
}

void SettingsManager::add(
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