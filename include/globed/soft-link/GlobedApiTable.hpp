#pragma once

#include "FunctionTable.hpp"
#include "../core/net/ConnectionState.hpp"
#include "../core/data/UserRole.hpp"
#include "../core/data/Event.hpp"
#include "../core/data/FeaturedLevel.hpp"
#include "../core/data/PlayerIconData.hpp"

#include <Geode/utils/function.hpp>
#include "../core/net/MessageListener.hpp"

#define API_TABLE_FN(...) GEODE_INVOKE(GEODE_CONCAT(API_TABLE_FN_, GEODE_NUMBER_OF_ARGS(__VA_ARGS__)), __VA_ARGS__)

#define API_TABLE_FN_2(ret, name) \
    inline ::geode::Result<ret> name() { return this->invoke<ret>(#name); }
#define API_TABLE_FN_3(ret, name, p1) \
    inline ::geode::Result<ret> name(p1 a1) { return this->invoke<ret>(#name, a1); }
#define API_TABLE_FN_4(ret, name, p1, p2) \
    inline ::geode::Result<ret> name(p1 a1, p2 a2) { return this->invoke<ret>(#name, a1, a2); }
#define API_TABLE_FN_5(ret, name, p1, p2, p3) \
    inline ::geode::Result<ret> name(p1 a1, p2 a2, p3 a3) { return this->invoke<ret>(#name, a1, a2, a3); }
#define API_TABLE_FN_6(ret, name, p1, p2, p3, p4) \
    inline ::geode::Result<ret> name(p1 a1, p2 a2, p3 a3, p4 a4) { return this->invoke<ret>(#name, a1, a2, a3, a4); }
#define API_TABLE_FN_7(ret, name, p1, p2, p3, p4, p5) \
    inline ::geode::Result<ret> name(p1 a1, p2 a2, p3 a3, p4 a4, p5 a5) { return this->invoke<ret>(#name, a1, a2, a3, a4, a5); }
#define API_TABLE_FN_8(ret, name, p1, p2, p3, p4, p5, p6) \
    inline ::geode::Result<ret> name(p1 a1, p2 a2, p3 a3, p4 a4, p5 a5, p6 a6) { return this->invoke<ret>(#name, a1, a2, a3, a4, a5, a6); }
#define API_TABLE_FN_9(ret, name, p1, p2, p3, p4, p5, p6, p7) \
    inline ::geode::Result<ret> name(p1 a1, p2 a2, p3 a3, p4 a4, p5 a5, p6 a6, p7 a7) { return this->invoke<ret>(#name, a1, a2, a3, a4, a5, a6, a7); }
#define API_TABLE_FN_10(ret, name, p1, p2, p3, p4, p5, p6, p7, p8) \
    inline ::geode::Result<ret> name(p1 a1, p2 a2, p3 a3, p4 a4, p5 a5, p6 a6, p7 a7, p8 a8) { return this->invoke<ret>(#name, a1, a2, a3, a4, a5, a6, a7, a8); }
#define API_TABLE_FN_11(ret, name, p1, p2, p3, p4, p5, p6, p7, p8, p9) \
    inline ::geode::Result<ret> name(p1 a1, p2 a2, p3 a3, p4 a4, p5 a5, p6 a6, p7 a7, p8 a8, p9 a9) { return this->invoke<ret>(#name, a1, a2, a3, a4, a5, a6, a7, a8, a9); }

namespace globed {

struct GlobedApiTable;
class RemotePlayer;
class VisualPlayer;

struct NetSubtable : public FunctionTableSubcat<GlobedApiTable> {
    API_TABLE_FN(void, disconnect);
    API_TABLE_FN(bool, isConnected);
    API_TABLE_FN(ConnectionState, getConnectionState);

    API_TABLE_FN(uint32_t, getCentralPingMs);
    API_TABLE_FN(uint32_t, getGamePingMs);
    API_TABLE_FN(uint32_t, getGameTickrate);

    API_TABLE_FN(std::vector<UserRole>, getAllRoles);
    API_TABLE_FN(std::vector<UserRole>, getUserRoles);
    API_TABLE_FN(std::vector<uint8_t>, getUserRoleIds);
    API_TABLE_FN(std::optional<UserRole>, getUserHighestRole);
    API_TABLE_FN(std::optional<UserRole>, findRole, uint8_t /* numeric id */);
    API_TABLE_FN(std::optional<UserRole>, findRoleStr, std::string_view /* string id */);
    API_TABLE_FN(bool, isModerator);
    API_TABLE_FN(bool, isAuthorizedModerator);

    API_TABLE_FN(void, invalidateIcons);
    API_TABLE_FN(void, invalidateFriendList);

    API_TABLE_FN(std::optional<FeaturedLevelMeta>, getFeaturedLevel);
    API_TABLE_FN(void, queueGameEvent, OutEvent&&);
    API_TABLE_FN(void, sendQuickChat, uint32_t /* emote id */);

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

struct GameSubtable : public FunctionTableSubcat<GlobedApiTable> {
    /// Returns whether the user is currently in a level and Globed is active
    API_TABLE_FN(bool, isActive);
    /// Returns if Globed safe mode is enabled (e.g. because of certain room modifiers)
    API_TABLE_FN(bool, isSafeMode);
    /// Kills the local player, pass 'true' to fake the death (won't trigger deaths of others in death link for example)
    API_TABLE_FN(void, killLocalPlayer, bool /* fake */);
    /// Cancels automatic respawn after death
    API_TABLE_FN(void, cancelLocalRespawn);
    /// Respawns the player, resetting the level
    API_TABLE_FN(void, causeLocalRespawn, bool /* full reset */);
    /// Enables safe mode until the next player death.
    /// Use `enablePermanentSafeMode` for a safe mode that lasts until player leaves the level.
    API_TABLE_FN(void, enableSafeMode);
    /// Resets safe mode. This does nothing if `enablePermanentSafeMode` was called before.
    API_TABLE_FN(void, resetSafeMode);
    /// Enables safe mode until the player leaves the level fully.
    API_TABLE_FN(void, enablePermanentSafeMode);
    /// Enable / disable whether player culling is enabled. By default, it is enabled.
    /// Disabling it can help in certain gamemodes, but it will use more bandwidth and CPU usage.
    API_TABLE_FN(void, toggleCullingEnabled, bool /* culling */);
    /// Enable / disable whether extra data is sent for players. By default, is disabled.
    API_TABLE_FN(void, toggleExtendedData, bool /* extended */);

    /// Plays an emote given an emote ID. Returns false if on cooldown or the emote is invalid.
    API_TABLE_FN(bool, playSelfEmote, uint32_t /* emote id */);
    /// Plays a favorite emote given its slot number (0 to 7)
    API_TABLE_FN(bool, playSelfFavoriteEmote, uint32_t /* which */);

    /// Returns a RemotePlayer* object for the given player ID.
    /// It should be treated in an opaque way and only used with the player functions.
    API_TABLE_FN(RemotePlayer*, getPlayer, int /* player id */);
};

struct PlayerSubtable : public FunctionTableSubcat<GlobedApiTable> {
    /// Returns whether the given PlayerObject* belongs to Globed.
    /// This is a quick, inlined tag check and may lead to false positives if someone else uses the same tag.
    inline bool isGlobedPlayerFast(PlayerObject* player) {
        return player && player->getTag() == 3458738;
    }

    /// Returns whether the given PlayerObject* *certainly* belongs to Globed.
    API_TABLE_FN(bool, isGlobedPlayer, PlayerObject*);

    /// Returns the player 1 of this RemotePlayer.
    /// Unlike RemotePlayer, VisualPlayer is a node, and if you don't want to include the VisualPlayer.hpp header,
    /// it is safe to cast it to CCNode* and use as such.
    API_TABLE_FN(VisualPlayer*, getFirst, RemotePlayer*);
    /// Returns the player 2 of this RemotePlayer.
    API_TABLE_FN(VisualPlayer*, getSecond, RemotePlayer*);
    /// Returns the RemotePlayer belonging to this VisualPlayer*.
    /// If you only have a PlayerObject*, you can first call `isGlobedPlayer` to ensure it's a Globed player,
    /// and then cast and call this function.
    API_TABLE_FN(RemotePlayer*, getRemote, VisualPlayer*);

    /// Checks whether this player is a teammate. This only works if in a room with teams enabled.
    /// When not in such a room, this will return true for every player.
    API_TABLE_FN(bool, isTeammate, RemotePlayer*);
    /// Returns the account ID of this player.
    API_TABLE_FN(int, getAccountId, RemotePlayer*);
    /// Returns the username of this player.
    API_TABLE_FN(std::string, getUsername, RemotePlayer*);
    /// Returns the icons of this player.
    API_TABLE_FN(const PlayerIconData&, getIcons, RemotePlayer*);

    /// Set whether this player should be forcibly hidden
    API_TABLE_FN(void, setForceHide, RemotePlayer*, bool /* hidden */);
};

struct GlobedApiTable : public FunctionTable {
    NetSubtable net{this, "net"};
    GameSubtable game{this, "game"};
    PlayerSubtable player{this, "player"};
};

}

#undef VA_NARGS_IMPL
#undef VA_NARGS
#undef API_TABLE_FN_0
#undef API_TABLE_FN_1
#undef API_TABLE_FN_2
#undef API_TABLE_FN_3
#undef API_TABLE_FN_4
#undef API_TABLE_FN_5
#undef API_TABLE_FN_6
#undef API_TABLE_FN_7
#undef API_TABLE_FN_8
#undef API_TABLE_FN_9
#undef API_TABLE_FN_10
#undef API_TABLE_FN
