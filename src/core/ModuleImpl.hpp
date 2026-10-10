#pragma once
#include <globed/core/Module.hpp>
#include <globed/prelude.hpp>

namespace globed {

class ModuleImpl {
    struct ctor_tag {};
public:
    static Result<std::shared_ptr<ModuleImpl>> create(ModuleVTable* vtable);
    ModuleImpl(ModuleVTable* vtable, ctor_tag) : m_vtable(std::move(vtable)) {}

    std::string_view id() const;

    AutoEnableMode getAutoEnableMode() const;
    void setAutoEnableMode(AutoEnableMode mode);

    bool isEnabled() const;
    Result<> enable();
    Result<> disable();

    void onRegistered();

    void claimHooks(std::span<geode::Hook* const> hooks);
    void claimPatches(std::span<geode::Patch* const> patches);

    void onJoinLevel(GlobedGJBGL* gjbgl, GJGameLevel* level, bool editor);
    void onJoinLevelPostInit(GlobedGJBGL* gjbgl);

    // Call when leaving a level, disables all modules that have auto enable set to Level
    void onLeaveLevel(GlobedGJBGL* gjbgl, bool editor);

    // Call when another player joins the level, calls `onPlayerJoin` on each enabled module
    void onPlayerJoin(GlobedGJBGL* gjbgl, int accountId);
    // Call when another player leaves the level, calls `onPlayerLeave` on each enabled module
    void onPlayerLeave(GlobedGJBGL* gjbgl, int accountId);
    // Call when another player dies, calls `onPlayerDeath` on each enabled module
    void onPlayerDeath(GlobedGJBGL* gjbgl, RemotePlayer* player, const PlayerDeath& death);
    // Call when another player respawns, calls `onPlayerRespawn` on each enabled module
    void onPlayerRespawn(GlobedGJBGL* gjbgl, RemotePlayer* player);

    bool shouldSpeedUpNewBest(GlobedGJBGL* gjbgl);
    void onLocalPlayerDeath(GlobedGJBGL* gjbgl, bool real);
    void onUpdate(GlobedGJBGL* gjbgl, float dt);
    void onPreUpdate(GlobedGJBGL* gjbgl, float dt);

    bool wantsSyncReset();


private:
    std::string m_id;
    ModuleVTable* m_vtable;
    std::unordered_set<geode::Hook*> m_hooks;
    std::unordered_set<geode::Patch*> m_patches;
    AutoEnableMode m_autoEnableMode = AutoEnableMode::Default;
    bool m_enabled = false;
};

}
