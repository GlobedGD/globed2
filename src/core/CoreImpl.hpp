#pragma once

#include <globed/core/Module.hpp>
#include <globed/core/Core.hpp>
#include <globed/core/data/Messages.hpp>
#include <globed/core/net/MessageListener.hpp>
#include <core/game/Interpolator.hpp>
#include <memory>

#include <Geode/utils/function.hpp>

namespace globed {

class RemotePlayer;

class CoreImpl {
public:
    CoreImpl();
    ~CoreImpl();

    static CoreImpl& get() {
        return *Core::get().m_impl;
    }

    geode::Result<> addModule(std::shared_ptr<Module> mod);
    Module* findModule(std::string_view id);

    // Call at the end of the loading phase, enables all modules that have auto enable set to Launch
    void onLaunch();

    // Call when connected to a server, enables all modules that have auto enable set to Server
    void onServerConnected();

    // Call when disconnected from a server, disables all modules that have auto enable set to Server
    void onServerDisconnected();

    // Call when joining a level while connected to a server, enables all modules that have auto enable set to Level
    // Also calls `onJoinLevel` on each enabled module
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
    void onUserlistSetup(cocos2d::CCNode* container, int accountId, bool myself, UserListPopup* popup);

    bool shouldSpeedUpNewBest(GlobedGJBGL* gjbgl);
    void onLocalPlayerDeath(GlobedGJBGL* gjbgl, bool real);
    void onUpdate(GlobedGJBGL* gjbgl, float dt);
    void onPreUpdate(GlobedGJBGL* gjbgl, float dt);

    bool wantsSyncReset();

private:
    std::vector<std::shared_ptr<Module>> m_modules;

    void enableIf(geode::FunctionRef<bool(Module&)>&& func);
    void disableIf(geode::FunctionRef<bool(Module&)>&& func);
    void forEachEnabled(geode::FunctionRef<void(Module&)>&& func);
};

}