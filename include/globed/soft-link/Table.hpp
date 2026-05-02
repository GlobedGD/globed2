#pragma once

#include "../core/net/ConnectionState.hpp"
#include "../core/net/MessageListener.hpp"
#include "../core/data/UserRole.hpp"
#include "../core/data/Event.hpp"
#include "../core/data/FeaturedLevel.hpp"
#include "../core/data/PlayerIconData.hpp"
#include "../core/data/RoomSettings.hpp"
#include "../core/data/RoomTeam.hpp"
#include "../core/SessionId.hpp"
#include "../util/vtable.hpp"

#include <Geode/utils/function.hpp>
#include <Geode/loader/Dispatch.hpp>

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
    GLOBED_VTABLE_FUNC(findRole, std::optional<UserRole>, uint8_t numericId);
    GLOBED_VTABLE_FUNC(findRoleStr, std::optional<UserRole>, std::string_view stringId);
    GLOBED_VTABLE_FUNC(isModerator, bool);
    GLOBED_VTABLE_FUNC(isAuthorizedModerator, bool);

    GLOBED_VTABLE_FUNC(invalidateIcons, void);
    GLOBED_VTABLE_FUNC(invalidateFriendList, void);

    GLOBED_VTABLE_FUNC(getFeaturedLevel, std::optional<FeaturedLevelMeta>);
    GLOBED_VTABLE_FUNC(queueGameEvent, void, void*); // dont use

    // since v2.2.0
    GLOBED_VTABLE_FUNC(sendEvent, void, std::string_view id, std::vector<uint8_t> data, const EventOptions& options);
    GLOBED_VTABLE_FUNC(registerEvent, void, std::string_view id, EventServer server);
};

struct GameSubtable : VTable {
    GameSubtable();
    GLOBED_VTABLE_FUNC(isActive, bool);
    GLOBED_VTABLE_FUNC(isSafeMode, bool);
    GLOBED_VTABLE_FUNC(killLocalPlayer, void, bool fake);

    GLOBED_VTABLE_FUNC(cancelLocalRespawn, void);
    GLOBED_VTABLE_FUNC(causeLocalRespawn, void, bool fullReset);
    GLOBED_VTABLE_FUNC(enableSafeMode, void);
    GLOBED_VTABLE_FUNC(resetSafeMode, void);
    GLOBED_VTABLE_FUNC(enablePermanentSafeMode, void);
    GLOBED_VTABLE_FUNC(toggleCullingEnabled, void, bool culling);
    GLOBED_VTABLE_FUNC(toggleExtendedData, void, bool extended);

    GLOBED_VTABLE_FUNC(playSelfEmote, bool, uint32_t emoteId);
    GLOBED_VTABLE_FUNC(playSelfFavoriteEmote, bool, uint32_t which);

    GLOBED_VTABLE_FUNC(getPlayer, RemotePlayer*, int player);

    // since v2.0.1
    GLOBED_VTABLE_FUNC(updateLocalIcons, void, std::optional<PlayerIconData>);

    // since v2.1.0
    GLOBED_VTABLE_FUNC(getPlayerIds, std::vector<int>);
    GLOBED_VTABLE_FUNC(getPlayers, std::vector<std::shared_ptr<RemotePlayer>>);
    GLOBED_VTABLE_FUNC(getPlayerCount, size_t);
};

struct PlayerSubtable : VTable {
    PlayerSubtable();
    GLOBED_VTABLE_FUNC(isGlobedPlayer, bool, PlayerObject*);

    GLOBED_VTABLE_FUNC(getFirst, VisualPlayer*, RemotePlayer*);
    GLOBED_VTABLE_FUNC(getSecond, VisualPlayer*, RemotePlayer*);
    GLOBED_VTABLE_FUNC(getRemote, RemotePlayer*, VisualPlayer*);

    GLOBED_VTABLE_FUNC(isTeammate, bool, RemotePlayer*);
    GLOBED_VTABLE_FUNC(getAccountId, int, RemotePlayer*);
    GLOBED_VTABLE_FUNC(getUsername, std::string, RemotePlayer*);
    GLOBED_VTABLE_FUNC(getIcons, geode::Result<PlayerIconData>, RemotePlayer*);

    GLOBED_VTABLE_FUNC(setForceHide, void, RemotePlayer*, bool hidden);
};

struct MiscSubtable : VTable {
    MiscSubtable();
    GLOBED_VTABLE_FUNC(fullPathForFilename, gd::string, std::string_view, bool);
};

// Since v2.2.0 (entire table is null before)
struct RoomSubtable : VTable {
    RoomSubtable();

    GLOBED_VTABLE_FUNC(isInRoom, bool);
    GLOBED_VTABLE_FUNC(getId, uint32_t);
    GLOBED_VTABLE_FUNC(isOwner, bool);
    GLOBED_VTABLE_FUNC(getOwner, int);
    GLOBED_VTABLE_FUNC(pickServerId, std::optional<uint8_t>);
    GLOBED_VTABLE_FUNC(getSettings, RoomSettings*);
    GLOBED_VTABLE_FUNC(getPasscode, uint32_t);
    GLOBED_VTABLE_FUNC(getPinnedLevel, SessionId);
    GLOBED_VTABLE_FUNC(getCurrentWarpLevel, SessionId);

    GLOBED_VTABLE_FUNC(getCurrentTeamId, uint16_t);
    GLOBED_VTABLE_FUNC(getCurrentTeam, std::optional<RoomTeam>);
    GLOBED_VTABLE_FUNC(getTeam, std::optional<RoomTeam>, uint16_t id);
    GLOBED_VTABLE_FUNC(getTeamIdForPlayer, std::optional<uint16_t>, int playerId);

    GLOBED_VTABLE_FUNC(makeSessionId, SessionId, int levelId);
};

struct RootApiTable {
    RootApiTable(); // intentionally not dllexported

    NetSubtable* net = nullptr;
    GameSubtable* game = nullptr;
    PlayerSubtable* player = nullptr;
    MiscSubtable* misc = nullptr;
    RoomSubtable* room = nullptr;
    void* _reserved[64 - 5] = {nullptr};
};

static_assert(sizeof(RootApiTable) == sizeof(void*) * 64);


#undef MY_MOD_ID
#define MY_MOD_ID "dankmeme.globed2"

#if !defined GLOBED_BUILD || defined GEODE_DEFINE_EVENT_EXPORTS
inline geode::Result<RootApiTable*> getRootTable() GEODE_EVENT_EXPORT(&getRootTable, ());
#else
geode::Result<RootApiTable*> getRootTable();
#endif

}
