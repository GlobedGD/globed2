#pragma once
#include <string>
#include "../util/vtable.hpp"

class GJGameLevel;

namespace globed {

class RemotePlayer;
struct GlobedGJBGL;
struct PlayerDeath;

struct ModuleMetadata {
    std::string id;
    std::string name;
    std::string author;
};

enum class AutoEnableMode {
    /// The module will never be automatically enabled. You must call `enable()` manually.
    Never,
    /// The module will be enabled when the game loads, and never disabled or re-enabled.
    Launch,
    /// The module will be enabled when the user connects to a server, and will be disabled when the user disconnects.
    /// This is the default mode.
    Server,
    /// The module will be enabled when the user joins a level while connected to a server, and will be disabled when the user leaves the level or disconnects.
    /// Useful for modules that only need to be active when in a level.
    Level,

    /// Chooses the default mode, currently `Server`.
    Default = Server
};

/// Vtable that every module must define and give to globed, it has callbacks for metadata and other events mods can subscribe to
struct ModuleVTable : VTable {
    constexpr ModuleVTable() noexcept : VTable(sizeof(ModuleVTable)) {}

    GLOBED_VTABLE_FUNC(getMetadata, ModuleMetadata);
    GLOBED_VTABLE_FUNC(onRegistered, void);
    GLOBED_VTABLE_FUNC(onEnabled, geode::Result<>);
    GLOBED_VTABLE_FUNC(onDisabled, void);

    GLOBED_VTABLE_FUNC(onJoinLevel, void, GlobedGJBGL* gjbgl, GJGameLevel* level, bool editor);
    GLOBED_VTABLE_FUNC(onJoinLevelPostInit, void, GlobedGJBGL* gjbgl);
    GLOBED_VTABLE_FUNC(onLeaveLevel, void, GlobedGJBGL* gjbgl, bool editor);

    GLOBED_VTABLE_FUNC(onPlayerJoin, void, GlobedGJBGL* gjbgl, int accountId);
    GLOBED_VTABLE_FUNC(onPlayerLeave, void, GlobedGJBGL* gjbgl, int accountId);
    GLOBED_VTABLE_FUNC(onPlayerDeath, void, GlobedGJBGL* gjbgl, RemotePlayer* player, const PlayerDeath& death);
    GLOBED_VTABLE_FUNC(onPlayerRespawn, void, GlobedGJBGL* gjbgl, RemotePlayer* player);

    GLOBED_VTABLE_FUNC(wantsSyncReset, bool);

    GLOBED_VTABLE_FUNC(onUpdate, void, GlobedGJBGL* gjbgl, float dt);
    GLOBED_VTABLE_FUNC(onPreUpdate, void, GlobedGJBGL* gjbgl, float dt);

    GLOBED_VTABLE_FUNC(shouldSpeedUpNewBest, bool, GlobedGJBGL* gjbgl);
    GLOBED_VTABLE_FUNC(onLocalPlayerDeath, void, GlobedGJBGL* gjbgl, bool real);
};

}
