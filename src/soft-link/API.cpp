#define GEODE_DEFINE_EVENT_EXPORTS

#include <globed/soft-link/API.hpp>
#include <globed/core/net/NetworkManager.hpp>
#include <core/hooks/GJBaseGameLayer.hpp>
#include <core/net/NetworkManagerImpl.hpp>
#include <core/preload/PreloadManager.hpp>
#include <asp/format.hpp>

using namespace geode::prelude;

namespace globed {

static GlobedApiTable s_functionTable;

Result<FunctionTable*> getFunctionTable() {
    return Ok(&s_functionTable);
}

struct Assigner {
    std::string_view cat, name;

    template <typename T>
    Assigner& operator=(T&& func) {
        char buf[128];
        auto fullName = asp::local_format(buf, "{}.{}", cat, name);

#ifdef GLOBED_DEBUG
        log::trace("Adding soft api function: '{}'", fullName);
#endif

        s_functionTable.insert(fullName, std::forward<T>(func));
        return *this;
    }
};

static auto assigner(std::string_view cat, std::string_view name) {
    return Assigner{ cat, name };
}

#define TABLE_FN(ret, name, ...) \
    assigner(category, #name) = +[](__VA_ARGS__) -> Result<ret>

static void addNetFunctions() {
    auto category = "net";

    TABLE_FN(void, disconnect) {
        NetworkManagerImpl::get().disconnectCentral();
        return Ok();
    };

    TABLE_FN(bool, isConnected) {
        return Ok(NetworkManagerImpl::get().isConnected());
    };

    TABLE_FN(ConnectionState, getConnectionState) {
        return Ok(NetworkManager::get().getConnectionState());
    };

    TABLE_FN(uint32_t, getCentralPingMs) {
        return Ok(NetworkManagerImpl::get().getCentralPing().millis());
    };

    TABLE_FN(uint32_t, getGamePingMs) {
        return Ok(NetworkManagerImpl::get().getGamePing().millis());
    };

    TABLE_FN(uint32_t, getGameTickrate) {
        return Ok(NetworkManagerImpl::get().getGameTickrate());
    };


    TABLE_FN(std::vector<UserRole>, getAllRoles) {
        return Ok(NetworkManagerImpl::get().getAllRoles());
    };

    TABLE_FN(std::vector<UserRole>, getUserRoles) {
        return Ok(NetworkManagerImpl::get().getUserRoles());
    };

    TABLE_FN(std::vector<uint8_t>, getUserRoleIds) {
        return Ok(NetworkManagerImpl::get().getUserRoleIds());
    };

    TABLE_FN(std::optional<UserRole>, getUserHighestRole) {
        return Ok(NetworkManagerImpl::get().getUserHighestRole());
    };

    TABLE_FN(std::optional<UserRole>, findRole, uint8_t id) {
        return Ok(NetworkManagerImpl::get().findRole(id));
    };

    TABLE_FN(std::optional<UserRole>, findRoleStr, std::string_view id) {
        return Ok(NetworkManagerImpl::get().findRole(id));
    };

    TABLE_FN(bool, isModerator) {
        return Ok(NetworkManagerImpl::get().isModerator());
    };

    TABLE_FN(bool, isAuthorizedModerator) {
        return Ok(NetworkManagerImpl::get().isAuthorizedModerator());
    };

    TABLE_FN(void, invalidateIcons) {
        NetworkManagerImpl::get().invalidateIcons();
        return Ok();
    };

    TABLE_FN(void, invalidateFriendList) {
        NetworkManagerImpl::get().invalidateFriendList();
        return Ok();
    };

    TABLE_FN(std::optional<FeaturedLevelMeta>, getFeaturedLevel) {
        return Ok(NetworkManagerImpl::get().getFeaturedLevel());
    };

    TABLE_FN(void, queueGameEvent, OutEvent&& event) {
        NetworkManagerImpl::get().queueGameEvent(std::move(event));
        return Ok();
    };

    TABLE_FN(void, sendQuickChat, uint32_t emoteId) {
        NetworkManagerImpl::get().sendQuickChat(emoteId);
        return Ok();
    };
}

static void addGameFunctions() {
    auto category = "game";

    TABLE_FN(bool, isActive) {
        auto gjbgl = GlobedGJBGL::getActive();
        return Ok(gjbgl);
    };

    TABLE_FN(bool, isSafeMode) {
        auto gjbgl = GlobedGJBGL::getActive();
        return Ok(gjbgl && gjbgl->isSafeMode());
    };

    TABLE_FN(void, killLocalPlayer, bool fake) {
        auto gjbgl = GlobedGJBGL::getActive();
        if (!gjbgl) return Err("Not in a level");

        gjbgl->killLocalPlayer(fake);
        return Ok();
    };

    TABLE_FN(void, cancelLocalRespawn) {
        auto gjbgl = GlobedGJBGL::getActive();
        if (!gjbgl) return Err("Not in a level");

        gjbgl->cancelLocalRespawn();
        return Ok();
    };

    TABLE_FN(void, causeLocalRespawn, bool full) {
        auto gjbgl = GlobedGJBGL::getActive();
        if (!gjbgl) return Err("Not in a level");

        gjbgl->causeLocalRespawn(full);
        return Ok();
    };

    TABLE_FN(void, enableSafeMode) {
        auto gjbgl = GlobedGJBGL::getActive();
        if (!gjbgl) return Err("Not in a level");

        gjbgl->enableSafeMode();
        return Ok();
    };

    TABLE_FN(void, resetSafeMode) {
        auto gjbgl = GlobedGJBGL::getActive();
        if (!gjbgl) return Err("Not in a level");

        gjbgl->resetSafeMode();
        return Ok();
    };

    TABLE_FN(void, enablePermanentSafeMode) {
        auto gjbgl = GlobedGJBGL::getActive();
        if (!gjbgl) return Err("Not in a level");

        gjbgl->setPermanentSafeMode();
        return Ok();
    };

    TABLE_FN(void, toggleCullingEnabled, bool culling) {
        auto gjbgl = GlobedGJBGL::getActive();
        if (!gjbgl) return Err("Not in a level");

        gjbgl->toggleCullingEnabled(culling);
        return Ok();
    };

    TABLE_FN(void, toggleExtendedData, bool extended) {
        auto gjbgl = GlobedGJBGL::getActive();
        if (!gjbgl) return Err("Not in a level");

        gjbgl->toggleExtendedData(extended);
        return Ok();
    };

    TABLE_FN(void, playSelfEmote, uint32_t id) {
        auto gjbgl = GlobedGJBGL::getActive();
        if (!gjbgl) return Err("Not in a level");

        gjbgl->playSelfEmote(id);
        return Ok();
    };

    TABLE_FN(void, playSelfFavoriteEmote, uint32_t which) {
        auto gjbgl = GlobedGJBGL::getActive();
        if (!gjbgl) return Err("Not in a level");

        gjbgl->playSelfFavoriteEmote(which);
        return Ok();
    };

    TABLE_FN(RemotePlayer*, getPlayer, int playerId) {
        auto gjbgl = GlobedGJBGL::getActive();
        if (!gjbgl || !gjbgl->active()) {
            return Ok(nullptr);
        }

        auto player = gjbgl->getPlayer(playerId);
        return Ok(player.get());
    };
}

static void addPlayerFunctions() {
    auto category = "player";

    TABLE_FN(bool, isGlobedPlayer, PlayerObject* player) {
        return Ok(typeinfo_cast<VisualPlayer*>(player) != nullptr);
    };

    TABLE_FN(VisualPlayer*, getFirst, RemotePlayer* rp) {
        if (!rp) return Err("null RemotePlayer was passed");
        return Ok(rp->player1());
    };

    TABLE_FN(VisualPlayer*, getSecond, RemotePlayer* rp) {
        if (!rp) return Err("null RemotePlayer was passed");
        return Ok(rp->player2());
    };

    TABLE_FN(RemotePlayer*, getRemote, VisualPlayer* vp) {
        if (!vp) return Err("null VisualPlayer was passed");
        return Ok(vp->getRemotePlayer());
    };

    TABLE_FN(bool, isTeammate, RemotePlayer* rp) {
        if (!rp) return Err("null RemotePlayer was passed");
        return Ok(rp->isTeammate(true));
    };

    TABLE_FN(int, getAccountId, RemotePlayer* rp) {
        if (!rp) return Err("null RemotePlayer was passed");
        return Ok(rp->id());
    };

    TABLE_FN(std::string, getUsername, RemotePlayer* rp) {
        if (!rp) return Err("null RemotePlayer was passed");
        return Ok(rp->displayData().username);
    };

    TABLE_FN(const PlayerIconData&, getIcons, RemotePlayer* rp) {
        if (!rp) return Err("null RemotePlayer was passed");
        return Ok(rp->displayData().icons);
    };

    TABLE_FN(void, setForceHide, RemotePlayer* rp, bool hide) {
        if (!rp) return Err("null RemotePlayer was passed");
        rp->setForceHide(hide);
        return Ok();
    };
}

static void addMiscFunctions() {
    auto category = "misc";

    TABLE_FN(gd::string, fullPathForFilename, std::string_view filename, bool ignoreSuffix) {
        return Ok(PreloadManager::get().fullPathForFilename(filename, ignoreSuffix));
    };
}

$execute {
    addNetFunctions();
    addGameFunctions();
    addPlayerFunctions();
    addMiscFunctions();

    // TODO more stuff

    s_functionTable.finalize();
}

}
