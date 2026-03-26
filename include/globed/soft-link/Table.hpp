#pragma once

#include "../core/net/ConnectionState.hpp"
#include "../core/data/UserRole.hpp"
#include "../core/data/Event.hpp"
#include "../core/data/FeaturedLevel.hpp"
#include "../core/data/PlayerIconData.hpp"
#include "../util/vtable.hpp"

#include <Geode/utils/function.hpp>
#include <Geode/loader/Dispatch.hpp>
#include "../core/net/MessageListener.hpp"

namespace globed {

class RemotePlayer;
class VisualPlayer;

struct NetSubtable : VTable {
    NetSubtable();
    GLOBED_VTABLE_FUNC(disconnect, void);
    GLOBED_VTABLE_FUNC(isConnected, bool);
    GLOBED_VTABLE_FUNC(getConnectionState, ConnectionState);

    GLOBED_VTABLE_FUNC(getCentralPingMs, uint32_t);
    GLOBED_VTABLE_FUNC(getGamePingMs, uint32_t);
    GLOBED_VTABLE_FUNC(getGameTickrate, uint32_t);

    GLOBED_VTABLE_FUNC(getAllRoles, std::vector<UserRole>);
    GLOBED_VTABLE_FUNC(getUserRoles, std::vector<UserRole>);
    GLOBED_VTABLE_FUNC(getUserRoleIds, std::vector<uint8_t>);
    GLOBED_VTABLE_FUNC(getUserHighestRole, std::optional<UserRole>);
    GLOBED_VTABLE_FUNC(findRole, std::optional<UserRole>, uint8_t /* numeric id */);
    GLOBED_VTABLE_FUNC(findRoleStr, std::optional<UserRole>, std::string_view /* string id */);
    GLOBED_VTABLE_FUNC(isModerator, bool);
    GLOBED_VTABLE_FUNC(isAuthorizedModerator, bool);

    GLOBED_VTABLE_FUNC(invalidateIcons, void);
    GLOBED_VTABLE_FUNC(invalidateFriendList, void);

    GLOBED_VTABLE_FUNC(getFeaturedLevel, std::optional<FeaturedLevelMeta>);
    GLOBED_VTABLE_FUNC(queueGameEvent, void, OutEvent&&);
    GLOBED_VTABLE_FUNC(sendQuickChat, void, uint32_t /* emote id */);

    // threadSafe = true means event will be invoked earlier and on the arc thread
    template <typename T, typename F>
    [[nodiscard("listen returns a listener that must be kept alive to receive messages")]]
    MessageListener<T> listen(F&& callback, int priority = 0, bool threadSafe = false) {
        return MessageEvent<T>(threadSafe).listen(std::forward<F>(callback), priority);
    }

    template <typename T, typename F>
    geode::ListenerHandle listenGlobal(F&& callback, int priority = 0, bool threadSafe = false) {
        return MessageEvent<T>(threadSafe).listen(std::forward<F>(callback), priority).leak();
    }
};

struct GameSubtable : VTable {
    GameSubtable();
    /// Returns whether the user is currently in a level and Globed is active
    GLOBED_VTABLE_FUNC(isActive, bool);
    /// Returns if Globed safe mode is enabled (e.g. because of certain room modifiers)
    GLOBED_VTABLE_FUNC(isSafeMode, bool);
    /// Kills the local player, pass 'true' to fake the death (won't trigger deaths of others in death link for example)
    GLOBED_VTABLE_FUNC(killLocalPlayer, void, bool /* fake */);
    /// Cancels automatic respawn after death
    GLOBED_VTABLE_FUNC(cancelLocalRespawn, void);
    /// Respawns the player, resetting the level
    GLOBED_VTABLE_FUNC(causeLocalRespawn, void, bool /* full reset */);
    /// Enables safe mode until the next player death.
    /// Use `enablePermanentSafeMode` for a safe mode that lasts until player leaves the level.
    GLOBED_VTABLE_FUNC(enableSafeMode, void);
    /// Resets safe mode. This does nothing if `enablePermanentSafeMode` was called before.
    GLOBED_VTABLE_FUNC(resetSafeMode, void);
    /// Enables safe mode until the player leaves the level fully.
    GLOBED_VTABLE_FUNC(enablePermanentSafeMode, void);
    /// Enable / disable whether player culling is enabled. By default, it is enabled.
    /// Disabling it can help in certain gamemodes, but it will use more bandwidth and CPU usage.
    GLOBED_VTABLE_FUNC(toggleCullingEnabled, void, bool /* culling */);
    /// Enable / disable whether extra data is sent for players. By default, is disabled.
    GLOBED_VTABLE_FUNC(toggleExtendedData, void, bool /* extended */);

    /// Plays an emote given an emote ID. Returns false if on cooldown or the emote is invalid.
    GLOBED_VTABLE_FUNC(playSelfEmote, bool, uint32_t /* emote id */);
    /// Plays a favorite emote given its slot number (0 to 7)
    GLOBED_VTABLE_FUNC(playSelfFavoriteEmote, bool, uint32_t /* which */);

    /// Returns a RemotePlayer* object for the given player ID.
    /// It should be treated in an opaque way and only used with the player functions.
    GLOBED_VTABLE_FUNC(getPlayer, RemotePlayer*, int /* player id */);
};

struct PlayerSubtable : VTable {
    PlayerSubtable();
    /// Returns whether the given PlayerObject* *certainly* belongs to Globed.
    GLOBED_VTABLE_FUNC(isGlobedPlayer, bool, PlayerObject*);

    /// Returns the player 1 of this RemotePlayer.
    /// Unlike RemotePlayer, VisualPlayer is a node, and if you don't want to include the VisualPlayer.hpp header,
    /// it is safe to cast it to CCNode* and use as such.
    GLOBED_VTABLE_FUNC(getFirst, VisualPlayer*, RemotePlayer*);
    /// Returns the player 2 of this RemotePlayer.
    GLOBED_VTABLE_FUNC(getSecond, VisualPlayer*, RemotePlayer*);
    /// Returns the RemotePlayer belonging to this VisualPlayer*.
    /// If you only have a PlayerObject*, you can first call `isGlobedPlayer` to ensure it's a Globed player,
    /// and then cast and call this function.
    GLOBED_VTABLE_FUNC(getRemote, RemotePlayer*, VisualPlayer*);

    /// Checks whether this player is a teammate. This only works if in a room with teams enabled.
    /// When not in such a room, this will return true for every player.
    GLOBED_VTABLE_FUNC(isTeammate, bool, RemotePlayer*);
    /// Returns the account ID of this player.
    GLOBED_VTABLE_FUNC(getAccountId, int, RemotePlayer*);
    /// Returns the username of this player.
    GLOBED_VTABLE_FUNC(getUsername, std::string, RemotePlayer*);
    /// Returns the icons of this player.
    GLOBED_VTABLE_FUNC(getIcons, geode::Result<PlayerIconData>, RemotePlayer*);

    /// Set whether this player should be forcibly hidden
    GLOBED_VTABLE_FUNC(setForceHide, void, RemotePlayer*, bool /* hidden */);
};

struct MiscSubtable : VTable {
    MiscSubtable();
    GLOBED_VTABLE_FUNC(fullPathForFilename, gd::string, std::string_view, bool);
};

struct RootApiTable {
    RootApiTable(); // intentionally not dllexported

    NetSubtable* net = nullptr;
    GameSubtable* game = nullptr;
    PlayerSubtable* player = nullptr;
    MiscSubtable* misc = nullptr;
    void* _reserved[64 - 4] = {nullptr};
};

static_assert(sizeof(RootApiTable) == sizeof(void*) * 64);


#undef MY_MOD_ID
#define MY_MOD_ID "dankmeme.globed2"

inline geode::Result<RootApiTable*> getRootTable() GEODE_EVENT_EXPORT(&getRootTable, ());

}
