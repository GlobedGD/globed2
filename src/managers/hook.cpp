#include "hook.hpp"

#include <globed/tracing.hpp>

namespace log = geode::log;

void HookManager::addHook(Group group, geode::Hook* hook) {
    groups[group].insert(HookData { hook });
}

std::set<HookManager::HookData>& HookManager::getHooks(Group group) {
    return groups[group];
}

void HookManager::enableGroup(Group group) {
    if (group == Group::EditorTriggerPopups) {
        disableGroup(group);
        return;
    }

    for (auto& h : groups[group]) {
        if (auto res = h.hook->enable()) {
            // TRACE("enabled hook {}", h.hook->getDisplayName());
        } else {
            log::warn("Failed to enable hook {}: {}", h.hook->getDisplayName(), res.unwrapErr());
        }
    }

    TRACE("enabled {} hooks in group {}", groups[group].size(), (int)group);
}

void HookManager::disableGroup(Group group) {
    for (auto& h : groups[group]) {
        if (auto res = h.hook->disable()) {
            // TRACE("disabled hook {}", h.hook->getDisplayName());
        } else {
            log::warn("Failed to disable hook {}: {}", h.hook->getDisplayName(), res.unwrapErr());
        }
    }

   TRACE("disabled {} hooks in group {}", groups[group].size(), (int)group);
}