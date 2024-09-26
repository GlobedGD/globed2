#pragma once

#include <util/singleton.hpp>
#include <Geode/loader/Hook.hpp>

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
        GlobalTriggers
    };

    std::map<Group, std::set<HookData>> groups;

    void addHook(Group group, geode::Hook* hook);
    std::set<HookData>& getHooks(Group group);

    void enableGroup(Group group);
    void disableGroup(Group group);
};
