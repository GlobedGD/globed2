#pragma once
#include <globed/util/singleton.hpp>
#include <globed/prelude.hpp>
#include "ModuleImpl.hpp"

namespace globed {

class GLOBED_DLL Core : public SingletonBase<Core> {
public:
    bool addModule(std::shared_ptr<ModuleImpl> module);
    ModuleImpl* findModule(std::string_view id);

    // Call at the end of the loading phase, enables all modules that have auto enable set to Launch
    void onLaunch();

    // Call when connected to a server, enables all modules that have auto enable set to Server
    void onServerConnected();

    // Call when disconnected from a server, disables all modules that have auto enable set to Server
    void onServerDisconnected();

    template <typename F, typename... Args>
    void invokeOnEnabled(F&& callback, Args&&... args) {
        this->forEachEnabled([&](ModuleImpl& module) {
            std::invoke(callback, module, args...); // no fwd
        });
    }

    // Call when joining a level while connected to a server, enables all modules that have auto enable set to Level
    // Also calls `onJoinLevel` on each enabled module
    void onJoinLevel(GlobedGJBGL* gjbgl, GJGameLevel* level, bool editor);

    void onJoinLevelPostInit(GlobedGJBGL* gjbgl) {
        return this->invokeOnEnabled(&ModuleImpl::onJoinLevelPostInit, gjbgl);
    }

    // Call when leaving a level, disables all modules that have auto enable set to Level
    void onLeaveLevel(GlobedGJBGL* gjbgl, bool editor);

    // Call when another player joins the level, calls `onPlayerJoin` on each enabled module
    void onPlayerJoin(GlobedGJBGL* gjbgl, int accountId) {
        return this->invokeOnEnabled(&ModuleImpl::onPlayerJoin, gjbgl, accountId);
    }
    // Call when another player leaves the level, calls `onPlayerLeave` on each enabled module
    void onPlayerLeave(GlobedGJBGL* gjbgl, int accountId) {
        return this->invokeOnEnabled(&ModuleImpl::onPlayerLeave, gjbgl, accountId);
    }
    // Call when another player dies, calls `onPlayerDeath` on each enabled module
    void onPlayerDeath(GlobedGJBGL* gjbgl, RemotePlayer* player, const PlayerDeath& death) {
        return this->invokeOnEnabled(&ModuleImpl::onPlayerDeath, gjbgl, player, death);
    }
    // Call when another player respawns, calls `onPlayerRespawn` on each enabled module
    void onPlayerRespawn(GlobedGJBGL* gjbgl, RemotePlayer* player) {
        return this->invokeOnEnabled(&ModuleImpl::onPlayerRespawn, gjbgl, player);
    }

    bool shouldSpeedUpNewBest(GlobedGJBGL* gjbgl);
    void onLocalPlayerDeath(GlobedGJBGL* gjbgl, bool real) {
        return this->invokeOnEnabled(&ModuleImpl::onLocalPlayerDeath, gjbgl, real);
    }
    void onUpdate(GlobedGJBGL* gjbgl, float dt) {
        return this->invokeOnEnabled(&ModuleImpl::onUpdate, gjbgl, dt);
    }
    void onPreUpdate(GlobedGJBGL* gjbgl, float dt) {
        return this->invokeOnEnabled(&ModuleImpl::onPreUpdate, gjbgl, dt);
    }

    bool wantsSyncReset();

private:
    friend SingletonBase<Core>;
    Core();

    geode::utils::StringMap<std::shared_ptr<ModuleImpl>> m_modules;

    void forEachEnabled(geode::FunctionRef<void(ModuleImpl&)> callback);
    void enableIf(geode::FunctionRef<bool(ModuleImpl&)>&& func);
    void disableIf(geode::FunctionRef<bool(ModuleImpl&)>&& func);
};

}
