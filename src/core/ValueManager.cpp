#include <globed/core/ValueManager.hpp>
#include <Geode/loader/Mod.hpp>

using namespace geode::prelude;

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

void ValueManager::eraseWithPrefix(std::string_view prefix) {
    this->eraseMatching([prefix](std::string_view key) {
        return key.starts_with(prefix);
    });
}

void ValueManager::eraseMatching(std23::function_ref<bool(std::string_view)> predicate) {
    auto& container = Mod::get()->getSaveContainer();

    std::vector<std::string> toErase;
    for (const auto& [key, _] : container) {
        if (predicate(key)) {
            toErase.push_back(key);
        }
    }

    for (const auto& key : toErase) {
        container.erase(key);
    }
}

}