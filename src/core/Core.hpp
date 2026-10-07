#pragma once
#include <globed/util/singleton.hpp>
#include <globed/prelude.hpp>
#include "ModuleImpl.hpp"

namespace globed {

class GLOBED_DLL Core : SingletonBase<Core> {
public:
    void addModule(std::shared_ptr<ModuleImpl> module);
    ModuleImpl* findModule(std::string_view id);

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

    bool shouldSpeedUpNewBest(GlobedGJBGL* gjbgl);
    void onLocalPlayerDeath(GlobedGJBGL* gjbgl, bool real);
    void onUpdate(GlobedGJBGL* gjbgl, float dt);
    void onPreUpdate(GlobedGJBGL* gjbgl, float dt);

    bool wantsSyncReset();

private:
    void forEachEnabled(geode::FunctionRef<void(Module&)> callback);
};

}
