#pragma once
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
            if (!mod->isOrWillBeEnabled()) {
                knownMissing = true;
            }
        }
    }

    return g_table;
}

bool available() {
    return table() != nullptr;
}

}

namespace api::net {

inline void disconnect() {
    if (auto t = table()) t->net->disconnect();
}

inline bool isConnected() {
    if (auto t = table()) return t->net->isConnected();
    return false;
}

inline ConnectionState getConnectionState() {
    if (auto t = table()) return t->net->getConnectionState();
    return ConnectionState::Disconnected;
}

inline uint32_t getCentralPingMs() {
    if (auto t = table()) return t->net->getCentralPingMs();
    return 0;
}

inline uint32_t getGamePingMs() {
    if (auto t = table()) return t->net->getGamePingMs();
    return 0;
}

inline uint32_t getGameTickrate() {
    if (auto t = table()) return t->net->getGameTickrate();
    return 0;
}

inline std::vector<UserRole> getAllRoles() {
    if (auto t = table()) return t->net->getAllRoles();
    return {};
}

inline std::vector<UserRole> getUserRoles() {
    if (auto t = table()) return t->net->getUserRoles();
    return {};
}

inline std::vector<uint8_t> getUserRoleIds() {
    if (auto t = table()) return t->net->getUserRoleIds();
    return {};
}

inline std::optional<UserRole> getUserHighestRole() {
    if (auto t = table()) return t->net->getUserHighestRole();
    return std::nullopt;
}

inline std::optional<UserRole> findRole(uint8_t id) {
    if (auto t = table()) return t->net->findRole(id);
    return std::nullopt;
}

inline std::optional<UserRole> findRoleStr(std::string_view id) {
    if (auto t = table()) return t->net->findRoleStr(id);
    return std::nullopt;
}

inline bool isModerator() {
    if (auto t = table()) return t->net->isModerator();
    return false;
}

inline bool isAuthorizedModerator() {
    if (auto t = table()) return t->net->isAuthorizedModerator();
    return false;
}

inline void invalidateIcons() {
    if (auto t = table()) t->net->invalidateIcons();
}

inline void invalidateFriendList() {
    if (auto t = table()) t->net->invalidateFriendList();
}

inline std::optional<FeaturedLevelMeta> getFeaturedLevel() {
    if (auto t = table()) return t->net->getFeaturedLevel();
    return std::nullopt;
}

inline void queueGameEvent(OutEvent&& event) {
    if (auto t = table()) t->net->queueGameEvent(std::move(event));
}

inline void sendQuickChat(uint32_t emoteId) {
    if (auto t = table()) t->net->sendQuickChat(emoteId);
}

} // namespace api::net

namespace api::game {

inline bool isActive() {
    if (auto t = table()) return t->game->isActive();
    return false;
}

inline bool isSafeMode() {
    if (auto t = table()) return t->game->isSafeMode();
    return false;
}

inline void killLocalPlayer(bool fake) {
    if (auto t = table()) t->game->killLocalPlayer(fake);
}

inline void cancelLocalRespawn() {
    if (auto t = table()) t->game->cancelLocalRespawn();
}

inline void causeLocalRespawn(bool fullReset) {
    if (auto t = table()) t->game->causeLocalRespawn(fullReset);
}

inline void enableSafeMode() {
    if (auto t = table()) t->game->enableSafeMode();
}

inline void resetSafeMode() {
    if (auto t = table()) t->game->resetSafeMode();
}

inline void enablePermanentSafeMode() {
    if (auto t = table()) t->game->enablePermanentSafeMode();
}

inline void toggleCullingEnabled(bool enabled) {
    if (auto t = table()) t->game->toggleCullingEnabled(enabled);
}

inline void toggleExtendedData(bool enabled) {
    if (auto t = table()) t->game->toggleExtendedData(enabled);
}

inline bool playSelfEmote(uint32_t id) {
    if (auto t = table()) return t->game->playSelfEmote(id);
    return false;
}

inline bool playSelfFavoriteEmote(uint32_t which) {
    if (auto t = table()) return t->game->playSelfFavoriteEmote(which);
    return false;
}

inline RemotePlayer* getPlayer(int playerId) {
    if (auto t = table()) return t->game->getPlayer(playerId);
    return nullptr;
}

} // namespace api::game

namespace api::player {

inline bool isGlobedPlayer(PlayerObject* player) {
    if (auto t = table()) return t->player->isGlobedPlayer(player);
    return false;
}

inline VisualPlayer* getFirst(RemotePlayer* player) {
    if (auto t = table()) return t->player->getFirst(player);
    return nullptr;
}

inline VisualPlayer* getSecond(RemotePlayer* player) {
    if (auto t = table()) return t->player->getSecond(player);
    return nullptr;
}

inline RemotePlayer* getRemote(VisualPlayer* player) {
    if (auto t = table()) return t->player->getRemote(player);
    return nullptr;
}

inline bool isTeammate(RemotePlayer* player) {
    if (auto t = table()) return t->player->isTeammate(player);
    return false;
}

inline int getAccountId(RemotePlayer* player) {
    if (auto t = table()) return t->player->getAccountId(player);
    return 0;
}

inline std::string getUsername(RemotePlayer* player) {
    if (auto t = table()) return t->player->getUsername(player);
    return "";
}

inline Result<PlayerIconData> getIcons(RemotePlayer* player) {
    if (auto t = table()) return t->player->getIcons(player);
    return Err("table not available");
}

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

}
