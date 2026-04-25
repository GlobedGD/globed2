#pragma once
#define GLOBED_SOFTLINK_INCLUDED
#include "Table.hpp"

namespace globed {

namespace api {

using geode::Result;
using geode::Ok;
using geode::Err;

inline RootApiTable* table() {
    static RootApiTable* g_table = nullptr;
    static bool knownMissing = false;

    if (!g_table && !knownMissing) {
        if (auto res = getRootTable()) {
            g_table = res.unwrap();
        } else {
            auto mod = geode::Loader::get()->getInstalledMod("dankmeme.globed2");
            if (!mod || !mod->isOrWillBeEnabled()) {
                knownMissing = true;
            }
        }
    }

    return g_table;
}

/// Returns whether globed is available and api functions can be used.
/// If this returns false, return values of api functions are meaningless.
bool available() {
    return table() != nullptr;
}

/// Returns whether the user has at least this version of Globed installed.
/// For example if v2.1.0 is passed, this will return true if the user has v2.1.0, v2.1.1, v2.2.0, v3.0.0, etc.
bool isAtLeast(std::string version) {
    auto mod = geode::Loader::get()->getInstalledMod("dankmeme.globed2");
    if (!mod) return false;

    auto globedVer = mod->getVersion();
    auto reqVer = geode::VersionInfo::parse(std::move(version));

    return reqVer && globedVer >= reqVer.unwrap();
}

/// Waits until Globed is loaded and invokes the callback. If Globed is already loaded, the callback is invoked immediately.
template <typename F>
void waitForGlobed(F&& callback) {
    if (geode::Loader::get()->isModLoaded("dankmeme.globed2")) {
        callback();
    } else {
        auto mod = geode::Loader::get()->getInstalledMod("dankmeme.globed2");
        if (!mod) return;

        geode::ModStateEvent(geode::ModEventType::Loaded, mod).listen([callback = std::forward<F>(callback)] mutable {
            callback();
        }).leak();
    }
}

}

namespace api::net {

/// Disconnects from the server
inline void disconnect() {
    if (auto t = table()) t->net->disconnect();
}

/// Returns whether currently connected to the (central) server
inline bool isConnected() {
    if (auto t = table()) return t->net->isConnected();
    return false;
}

/// Returns the current connection state
inline ConnectionState getConnectionState() {
    if (auto t = table()) return t->net->getConnectionState();
    return ConnectionState::Disconnected;
}

/// Returns the current ping to the central server in milliseconds. Returns 0 if not connected.
inline uint32_t getCentralPingMs() {
    if (auto t = table()) return t->net->getCentralPingMs();
    return 0;
}

/// Returns the current ping to the game server in milliseconds. Returns 0 if not connected.
inline uint32_t getGamePingMs() {
    if (auto t = table()) return t->net->getGamePingMs();
    return 0;
}

/// Returns the current game tickrate in ticks per second. Returns 0 if not connected.
inline uint32_t getGameTickrate() {
    if (auto t = table()) return t->net->getGameTickrate();
    return 0;
}

/// Returns a list of all user roles defined on the server.
inline std::vector<UserRole> getAllRoles() {
    if (auto t = table()) return t->net->getAllRoles();
    return {};
}

/// Returns a list of user roles the current user has.
inline std::vector<UserRole> getUserRoles() {
    if (auto t = table()) return t->net->getUserRoles();
    return {};
}

/// Returns a list of numeric IDs of user roles the current user has.
inline std::vector<uint8_t> getUserRoleIds() {
    if (auto t = table()) return t->net->getUserRoleIds();
    return {};
}

/// Returns the highest user role the current user has, or nullopt if the user has no roles or the information is unavailable.
inline std::optional<UserRole> getUserHighestRole() {
    if (auto t = table()) return t->net->getUserHighestRole();
    return std::nullopt;
}

/// Finds a user role by its numeric ID.
inline std::optional<UserRole> findRole(uint8_t id) {
    if (auto t = table()) return t->net->findRole(id);
    return std::nullopt;
}

/// Finds a user role by its string ID.
inline std::optional<UserRole> findRoleStr(std::string_view id) {
    if (auto t = table()) return t->net->findRoleStr(id);
    return std::nullopt;
}

/// Returns whether the current user is a moderator (has at least one moderator role).
inline bool isModerator() {
    if (auto t = table()) return t->net->isModerator();
    return false;
}

/// Returns whether the user is a moderator and has logged into the mod panel.
inline bool isAuthorizedModerator() {
    if (auto t = table()) return t->net->isAuthorizedModerator();
    return false;
}

/// Forces the icons of the local player to be re-sent to the central and game servers immediately.
/// This will also cause your icons to be updated live for all players, if in a session.
/// To update icons of the local player in other places (e.g. progress bar icons),
/// use `globed::api::game::updateLocalIcons()`
inline void invalidateIcons() {
    if (auto t = table()) t->net->invalidateIcons();
}

/// Forces the friend list to be re-sent to the central and game servers immediately.
inline void invalidateFriendList() {
    if (auto t = table()) t->net->invalidateFriendList();
}

inline std::optional<FeaturedLevelMeta> getFeaturedLevel() {
    if (auto t = table()) return t->net->getFeaturedLevel();
    return std::nullopt;
}

/// Queues an event, do not use this.
// inline void queueGameEvent(OutEvent&& event) {
//     if (auto t = table()) t->net->queueGameEvent(std::move(event));
// }

inline void sendEvent(std::string_view id, std::vector<uint8_t> data, const EventSendOptions& options) {
    if (auto t = table()) t->net->sendEvent(id, std::move(data), options);
}

inline void registerEvent(std::string_view id, EventServer server) {
    if (auto t = table()) t->net->registerEvent(id, server);
}

} // namespace api::net

namespace api::game {

/// Returns whether currently in a level and Globed is active (aka connected to game server)
inline bool isActive() {
    if (auto t = table()) return t->game->isActive();
    return false;
}

/// Returns if Globed safe mode is enabled (e.g. because of certain room modifiers)
inline bool isSafeMode() {
    if (auto t = table()) return t->game->isSafeMode();
    return false;
}

/// Kills the local player, pass 'true' to fake the death (won't trigger deaths of others in death link for example)
inline void killLocalPlayer(bool fake) {
    if (auto t = table()) t->game->killLocalPlayer(fake);
}

/// Cancels automatic respawn after death
inline void cancelLocalRespawn() {
    if (auto t = table()) t->game->cancelLocalRespawn();
}

/// Respawns the player, resetting the level
inline void causeLocalRespawn(bool fullReset) {
    if (auto t = table()) t->game->causeLocalRespawn(fullReset);
}

/// Enables safe mode until the next player death.
/// Use `enablePermanentSafeMode` for a safe mode that lasts until player leaves the level.
inline void enableSafeMode() {
    if (auto t = table()) t->game->enableSafeMode();
}

/// Resets safe mode. This does nothing if `enablePermanentSafeMode` was called before.
inline void resetSafeMode() {
    if (auto t = table()) t->game->resetSafeMode();
}

/// Enables safe mode until the player leaves the level fully.
inline void enablePermanentSafeMode() {
    if (auto t = table()) t->game->enablePermanentSafeMode();
}

/// Enable / disable whether player culling is enabled. By default, it is enabled.
/// Disabling it can help in certain gamemodes, but it will use more bandwidth and CPU usage.
inline void toggleCullingEnabled(bool enabled) {
    if (auto t = table()) t->game->toggleCullingEnabled(enabled);
}

/// Enable / disable whether extra data is sent for players. By default, is disabled.
inline void toggleExtendedData(bool enabled) {
    if (auto t = table()) t->game->toggleExtendedData(enabled);
}

/// Plays an emote given an emote ID. Returns false if on cooldown or the emote is invalid.
inline bool playSelfEmote(uint32_t id) {
    if (auto t = table()) return t->game->playSelfEmote(id);
    return false;
}

/// Plays a favorite emote given its slot number (0 to 7)
inline bool playSelfFavoriteEmote(uint32_t which) {
    if (auto t = table()) return t->game->playSelfFavoriteEmote(which);
    return false;
}

/// Returns a RemotePlayer* object for the given player ID.
/// It should be treated in an opaque way and only used with the player functions.
inline RemotePlayer* getPlayer(int playerId) {
    if (auto t = table()) return t->game->getPlayer(playerId);
    return nullptr;
}

/// Updates the player icons in the progress indicators (at screen edges and progress bar)
/// Uses the data from GameManager, to pass custom data use the other overload.
/// If custom data is passed, it will be reset back to GameManager data when re-entering the level.
/// Added in Globed v2.0.1.
inline void updateLocalIcons() {
    if (auto t = table()) t->game->updateLocalIcons(std::nullopt);
}

/// Updates the player icons in the progress indicators (at screen edges and progress bar)
/// Added in Globed v2.0.1.
inline void updateLocalIcons(PlayerIconData icons) {
    if (auto t = table()) t->game->updateLocalIcons(icons);
}

/// Returns IDs of all players that are in the same level as this user.
/// Returns an empty vector if not in an active session.
/// Added in Globed v2.1.0.
inline std::vector<int> getPlayerIds() {
    if (auto t = table()) return t->game->getPlayerIds();
    return {};
}

/// Returns RemotePlayer objects for all players that are in the same level as this user.
/// Returns an empty vector if not in an active session.
/// Added in Globed v2.1.0.
inline std::vector<std::shared_ptr<RemotePlayer>> getPlayers() {
    if (auto t = table()) return t->game->getPlayers();
    return {};
}

/// Returns the number of players that are in the same level as this user.
/// Returns 0 if not in an active session.
/// Added in Globed v2.1.0.
inline size_t getPlayerCount() {
    if (auto t = table()) return t->game->getPlayerCount();
    return 0;
}

} // namespace api::game

namespace api::player {

/// Returns whether the given PlayerObject likely belongs to Globed, without calling the API.
inline bool isGlobedPlayerFast(PlayerObject* player) {
    return player && player->getTag() == 3458738;
}

/// Returns whether the given PlayerObject* *certainly* belongs to Globed.
inline bool isGlobedPlayer(PlayerObject* player) {
    if (auto t = table()) return t->player->isGlobedPlayer(player);
    return false;
}

/// Returns the player 1 of this RemotePlayer.
/// Unlike RemotePlayer, VisualPlayer is a node, and if you don't want to include the VisualPlayer.hpp header,
/// it is safe to cast it to CCNode* and use as such.
inline VisualPlayer* getFirst(RemotePlayer* player) {
    if (auto t = table()) return t->player->getFirst(player);
    return nullptr;
}

/// Returns the player 2 of this RemotePlayer.
inline VisualPlayer* getSecond(RemotePlayer* player) {
    if (auto t = table()) return t->player->getSecond(player);
    return nullptr;
}

/// Returns the RemotePlayer belonging to this VisualPlayer*.
/// If you only have a PlayerObject*, you can first call `isGlobedPlayer` to ensure it's a Globed player,
/// and then cast and call this function.
inline RemotePlayer* getRemote(VisualPlayer* player) {
    if (auto t = table()) return t->player->getRemote(player);
    return nullptr;
}

/// Checks whether this player is a teammate. This only works if in a room with teams enabled.
/// When not in such a room, this will return true for every player.
inline bool isTeammate(RemotePlayer* player) {
    if (auto t = table()) return t->player->isTeammate(player);
    return false;
}

/// Returns the account ID of this player.
inline int getAccountId(RemotePlayer* player) {
    if (auto t = table()) return t->player->getAccountId(player);
    return 0;
}

/// Returns the username of this player.
inline std::string getUsername(RemotePlayer* player) {
    if (auto t = table()) return t->player->getUsername(player);
    return "";
}

/// Returns the icons of this player.
inline Result<PlayerIconData> getIcons(RemotePlayer* player) {
    if (auto t = table()) return t->player->getIcons(player);
    return Err("table not available");
}

/// Set whether this player should be forcibly hidden
inline void setForceHide(RemotePlayer* player, bool hidden) {
    if (auto t = table()) t->player->setForceHide(player, hidden);
}

} // namespace api::player

namespace api::misc {

inline gd::string fullPathForFilename(std::string_view filename, bool ignoreSuffix) {
    if (auto t = table()) return t->misc->fullPathForFilename(filename, ignoreSuffix);
    return gd::string{};
}

} // namespace api::misc

// we do a little sneaky

template <ValidEventType Derived>
void ServerEvent<Derived>::_register() {
    globed::api::waitForGlobed([] {
        globed::api::net::registerEvent(Derived::id(), Derived::server());
    });
}

template <ValidEventType Derived>
void ServerEvent<Derived>::send(this const Derived& self, const EventSendOptions& options) {
    globed::api::net::sendEvent(self.id(), self.encode(), options);
}

}
