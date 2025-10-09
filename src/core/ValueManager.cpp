#include <globed/core/ValueManager.hpp>
#include <Geode/loader/Mod.hpp>

using namespace geode::prelude;

// TODO (medium): bring back save slot support

namespace globed {

std::optional<matjson::Value> ValueManager::getValueRaw(std::string_view key) {
    if (Mod::get()->hasSavedValue(key)) {
        return Mod::get()->getSavedValue<matjson::Value>(key);
    } else {
        return std::nullopt;
    }
}

void ValueManager::set(std::string_view key, matjson::Value value) {
    Mod::get()->setSavedValue(key, value);
}

void ValueManager::erase(std::string_view key) {
    Mod::get()->getSaveContainer().erase(key);
}

}