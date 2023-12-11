#include "profile_cache.hpp"

ProfileCacheManager::ProfileCacheManager() {}

void ProfileCacheManager::insert(const FullPlayerMetadata& data) {
    cache[data.accountData.id] = data;
}

std::optional<FullPlayerMetadata> ProfileCacheManager::getData(int32_t accountId) {
    if (cache.contains(accountId)) {
        return cache.at(accountId);
    }

    return std::nullopt;
}

void ProfileCacheManager::clear() {
    cache.clear();
}

void ProfileCacheManager::setOwnDataAuto() {
    auto* gm = GameManager::get();

    PlayerIconData data(
        gm->m_playerFrame,
        gm->m_playerShip,
        gm->m_playerBall,
        gm->m_playerBird,
        gm->m_playerDart,
        gm->m_playerRobot,
        gm->m_playerSpider,
        0, // todo swing
        0, // TODO jetpack
        gm->m_playerDeathEffect,
        gm->m_playerColor,
        gm->m_playerColor2
    );

    this->setOwnData(data);
}

void ProfileCacheManager::setOwnData(const PlayerIconData& data) {
    if (ownData != data) {
        ownData = data;
        pendingChanges = true;
    }
}

PlayerIconData ProfileCacheManager::getOwnData() {
    return ownData;
}