#define GEODE_DEFINE_EVENT_EXPORTS

#include <globed/soft-link/API.hpp>
#include <globed/core/net/NetworkManager.hpp>
#include <core/hooks/GJBaseGameLayer.hpp>
#include <core/net/NetworkManagerImpl.hpp>
#include <core/preload/PreloadManager.hpp>
#include <asp/format.hpp>

using namespace geode::prelude;

namespace globed {

RootApiTable::RootApiTable() {}

NetSubtable::NetSubtable() : VTable(sizeof(NetSubtable)) {}
GameSubtable::GameSubtable() : VTable(sizeof(GameSubtable)) {}
PlayerSubtable::PlayerSubtable() : VTable(sizeof(PlayerSubtable)) {}
MiscSubtable::MiscSubtable() : VTable(sizeof(MiscSubtable)) {}

static RootApiTable* g_table = nullptr;

Result<RootApiTable*> getRootTable() {
    return Ok(g_table);
}

static NetSubtable* makeNetTable() {
    auto table = new NetSubtable;

    GLOBED_VTABLE_INIT(table, disconnect, () {
        NetworkManagerImpl::get().disconnectCentral();
    });

    GLOBED_VTABLE_INIT(table, isConnected, () {
        return NetworkManagerImpl::get().isConnected();
    });

    GLOBED_VTABLE_INIT(table, getConnectionState, () {
        return NetworkManager::get().getConnectionState();
    });

    GLOBED_VTABLE_INIT(table, getCentralPingMs, () -> uint32_t {
        return NetworkManagerImpl::get().getCentralPing().millis();
    });

    GLOBED_VTABLE_INIT(table, getGamePingMs, () -> uint32_t {
        return NetworkManagerImpl::get().getGamePing().millis();
    });

    GLOBED_VTABLE_INIT(table, getGameTickrate, () {
        return NetworkManagerImpl::get().getGameTickrate();
    });

    GLOBED_VTABLE_INIT(table, getAllRoles, () {
        return NetworkManagerImpl::get().getAllRoles();
    });

    GLOBED_VTABLE_INIT(table, getUserRoles, () {
        return NetworkManagerImpl::get().getUserRoles();
    });

    GLOBED_VTABLE_INIT(table, getUserRoleIds, () {
        return NetworkManagerImpl::get().getUserRoleIds();
    });

    GLOBED_VTABLE_INIT(table, getUserHighestRole, () {
        return NetworkManagerImpl::get().getUserHighestRole();
    });

    GLOBED_VTABLE_INIT(table, findRole, (uint8_t id) {
        return NetworkManagerImpl::get().findRole(id);
    });

    GLOBED_VTABLE_INIT(table, findRoleStr, (std::string_view id) {
        return NetworkManagerImpl::get().findRole(id);
    });

    GLOBED_VTABLE_INIT(table, isModerator, () {
        return NetworkManagerImpl::get().isModerator();
    });

    GLOBED_VTABLE_INIT(table, isAuthorizedModerator, () {
        return NetworkManagerImpl::get().isAuthorizedModerator();
    });

    GLOBED_VTABLE_INIT(table, invalidateIcons, () {
        NetworkManagerImpl::get().invalidateIcons(true);
    });

    GLOBED_VTABLE_INIT(table, invalidateFriendList, () {
        NetworkManagerImpl::get().invalidateFriendList();
    });

    GLOBED_VTABLE_INIT(table, getFeaturedLevel, () {
        return NetworkManagerImpl::get().getFeaturedLevel();
    });

    GLOBED_VTABLE_INIT(table, queueGameEvent, (OutEvent&& event) {
        // NetworkManagerImpl::get().queueGameEvent(std::move(event));
    });

    GLOBED_VTABLE_INIT(table, sendEvent, (std::string_view id, std::vector<uint8_t> data, const EventOptions& options) {
        NetworkManagerImpl::get().sendEvent(id, std::move(data), options);
    });

    GLOBED_VTABLE_INIT(table, registerEvent, (std::string_view id, EventServer server) {
        NetworkManagerImpl::get().registerEvent(id, server);
    });

    return table;
}

static GameSubtable* makeGameTable() {
    auto table = new GameSubtable;

    GLOBED_VTABLE_INIT(table, isActive, () {
        return (bool)GlobedGJBGL::getActive();
    });

    GLOBED_VTABLE_INIT(table, isSafeMode, () {
        auto gjbgl = GlobedGJBGL::getActive();
        return gjbgl && gjbgl->isSafeMode();
    });

    GLOBED_VTABLE_INIT(table, killLocalPlayer, (bool fake) {
        if (auto gjbgl = GlobedGJBGL::getActive()) gjbgl->killLocalPlayer(fake);
    });

    GLOBED_VTABLE_INIT(table, cancelLocalRespawn, () {
        if (auto gjbgl = GlobedGJBGL::getActive()) gjbgl->cancelLocalRespawn();
    });

    GLOBED_VTABLE_INIT(table, causeLocalRespawn, (bool full) {
        if (auto gjbgl = GlobedGJBGL::getActive()) gjbgl->causeLocalRespawn(full);
    });

    GLOBED_VTABLE_INIT(table, enableSafeMode, () {
        if (auto gjbgl = GlobedGJBGL::getActive()) gjbgl->enableSafeMode();
    });

    GLOBED_VTABLE_INIT(table, resetSafeMode, () {
        if (auto gjbgl = GlobedGJBGL::getActive()) gjbgl->resetSafeMode();
    });

    GLOBED_VTABLE_INIT(table, enablePermanentSafeMode, () {
        if (auto gjbgl = GlobedGJBGL::getActive()) gjbgl->setPermanentSafeMode();
    });

    GLOBED_VTABLE_INIT(table, toggleCullingEnabled, (bool culling) {
        if (auto gjbgl = GlobedGJBGL::getActive()) gjbgl->toggleCullingEnabled(culling);
    });

    GLOBED_VTABLE_INIT(table, toggleExtendedData, (bool extended) {
        if (auto gjbgl = GlobedGJBGL::getActive()) gjbgl->toggleExtendedData(extended);
    });

    GLOBED_VTABLE_INIT(table, playSelfEmote, (uint32_t id) {
        if (auto gjbgl = GlobedGJBGL::getActive()) return gjbgl->playSelfEmote(id);
        return false;
    });

    GLOBED_VTABLE_INIT(table, playSelfFavoriteEmote, (uint32_t which) {
        if (auto gjbgl = GlobedGJBGL::getActive()) return gjbgl->playSelfFavoriteEmote(which);
        return false;
    });

    GLOBED_VTABLE_INIT(table, getPlayer, (int playerId) -> RemotePlayer* {
        auto gjbgl = GlobedGJBGL::getActive();
        if (!gjbgl || !gjbgl->active()) return nullptr;
        return gjbgl->getPlayer(playerId).get();
    });

    GLOBED_VTABLE_INIT(table, updateLocalIcons, (std::optional<PlayerIconData> icons) {
        if (auto gjbgl = GlobedGJBGL::getActive()) gjbgl->updateLocalIcons(icons);
    });

    GLOBED_VTABLE_INIT(table, getPlayerIds, -> std::vector<int> {
        if (auto gjbgl = GlobedGJBGL::getActive()) {
            return asp::iter::keys(gjbgl->m_fields->m_players).collect();
        }
        return {};
    });

    GLOBED_VTABLE_INIT(table, getPlayers, -> std::vector<std::shared_ptr<RemotePlayer>> {
        if (auto gjbgl = GlobedGJBGL::getActive()) {
            return asp::iter::values(gjbgl->m_fields->m_players).collect();
        }
        return {};
    });

    GLOBED_VTABLE_INIT(table, getPlayerCount, -> size_t {
        if (auto gjbgl = GlobedGJBGL::getActive()) {
            return gjbgl->m_fields->m_players.size();
        }
        return 0;
    });

    return table;
}

static PlayerSubtable* makePlayerTable() {
    auto table = new PlayerSubtable;

    GLOBED_VTABLE_INIT(table, isGlobedPlayer, (PlayerObject* player) {
        return typeinfo_cast<VisualPlayer*>(player) != nullptr;
    });

    GLOBED_VTABLE_INIT(table, getFirst, (RemotePlayer* rp) -> VisualPlayer* {
        return rp ? rp->player1() : nullptr;
    });

    GLOBED_VTABLE_INIT(table, getSecond, (RemotePlayer* rp) -> VisualPlayer* {
        return rp ? rp->player2() : nullptr;
    });

    GLOBED_VTABLE_INIT(table, getRemote, (VisualPlayer* vp) -> RemotePlayer* {
        return vp ? vp->getRemotePlayer() : nullptr;
    });

    GLOBED_VTABLE_INIT(table, isTeammate, (RemotePlayer* rp) {
        return rp ? rp->isTeammate(true) : true;
    });

    GLOBED_VTABLE_INIT(table, getAccountId, (RemotePlayer* rp) {
        return rp ? rp->id() : 0;
    });

    GLOBED_VTABLE_INIT(table, getUsername, (RemotePlayer* rp) {
        return rp ? rp->displayData().username : std::string("");
    });

    GLOBED_VTABLE_INIT(table, getIcons, (RemotePlayer* rp) -> Result<PlayerIconData> {
        if (!rp) return Err("null RemotePlayer was passed");
        return Ok(rp->displayData().icons);
    });

    GLOBED_VTABLE_INIT(table, setForceHide, (RemotePlayer* rp, bool hide) {
        if (rp) rp->setForceHide(hide);
    });

    return table;
}

static MiscSubtable* makeMiscTable() {
    auto table = new MiscSubtable;

    GLOBED_VTABLE_INIT(table, fullPathForFilename, (std::string_view filename, bool ignoreSuffix) {
        return PreloadManager::get().fullPathForFilename(filename, ignoreSuffix);
    });

    return table;
}

$execute {
    g_table = new RootApiTable;
    g_table->net = makeNetTable();
    g_table->game = makeGameTable();
    g_table->player = makePlayerTable();
    g_table->misc = makeMiscTable();
}

}
