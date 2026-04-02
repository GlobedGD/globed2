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
    GLOBED_VTABLE_FUNC(isActive, bool);
    GLOBED_VTABLE_FUNC(isSafeMode, bool);
    GLOBED_VTABLE_FUNC(killLocalPlayer, void, bool /* fake */);

    GLOBED_VTABLE_FUNC(cancelLocalRespawn, void);
    GLOBED_VTABLE_FUNC(causeLocalRespawn, void, bool /* full reset */);
    GLOBED_VTABLE_FUNC(enableSafeMode, void);
    GLOBED_VTABLE_FUNC(resetSafeMode, void);
    GLOBED_VTABLE_FUNC(enablePermanentSafeMode, void);
    GLOBED_VTABLE_FUNC(toggleCullingEnabled, void, bool /* culling */);
    GLOBED_VTABLE_FUNC(toggleExtendedData, void, bool /* extended */);

    GLOBED_VTABLE_FUNC(playSelfEmote, bool, uint32_t /* emote id */);
    GLOBED_VTABLE_FUNC(playSelfFavoriteEmote, bool, uint32_t /* which */);

    GLOBED_VTABLE_FUNC(getPlayer, RemotePlayer*, int /* player id */);

    // since v5.0.1
    GLOBED_VTABLE_FUNC(updateLocalIcons, void, std::optional<PlayerIconData>);
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
