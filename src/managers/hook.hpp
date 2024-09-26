#pragma once

#include <util/singleton.hpp>
#include <Geode/loader/Hook.hpp>

#define GLOBED_MANAGE_HOOK(group, name) if (auto h = self.getHook(#name)) { ::HookManager::get().addHook(HookManager::Group::group, h.unwrap()); } else { log::warn("Cannot manage non-existent hook {}", #name); }

class HookManager : public SingletonBase<HookManager> {
private:
    friend class SingletonBase<HookManager>;

    struct HookData {
        geode::Hook* hook;

        std::strong_ordering operator<=>(const HookData& other) const = default;
    };

public:
    enum class Group {
        EditorTriggerPopups,
        GlobalTriggers,
        Gameplay, // All the hooks that should be disabled if not connected to a server.
    };

    std::map<Group, std::set<HookData>> groups;

    void addHook(Group group, geode::Hook* hook);
    std::set<HookData>& getHooks(Group group);

    void enableGroup(Group group);
    void disableGroup(Group group);
};
