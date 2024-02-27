#include "profile_cache.hpp"

void ProfileCacheManager::insert(const PlayerAccountData& data) {
    cache[data.accountId] = data;
}

std::optional<PlayerAccountData> ProfileCacheManager::getData(int32_t accountId) {
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
        gm->m_playerSwing,
        gm->m_playerJetpack,
        gm->m_playerDeathEffect,
        gm->m_playerColor,
        gm->m_playerColor2,
        gm->getPlayerGlow() ? gm->m_playerGlowColor.value() : -1
    );

    this->setOwnData(data);
}

void ProfileCacheManager::setOwnData(const PlayerIconData& data) {
    auto accountId = GJAccountManager::get()->m_accountID;
    auto& name = GJAccountManager::get()->m_username;

    bool changes = false;
    if (ownData.accountId != accountId) {
        ownData.accountId = accountId;
        changes = true;
    }

    if (std::string_view(ownData.name) != name) {
        ownData.name = name;
        changes = true;
    }

    if (ownData.icons != data) {
        ownData.icons = data;
        changes = true;
    }

    // no special user data
    ownData.specialUserData = ownSpecialData;

    pendingChanges = changes;
}

void ProfileCacheManager::setOwnSpecialData(const std::optional<SpecialUserData>& specialUserData) {
    ownSpecialData = specialUserData;
    ownData.specialUserData = ownSpecialData;
}

PlayerIconData ProfileCacheManager::getOwnData() {
    return ownData.icons;
}

PlayerAccountData& ProfileCacheManager::getOwnAccountData() {
    return ownData;
}

std::optional<SpecialUserData>& ProfileCacheManager::getOwnSpecialData() {
    return ownSpecialData;
}
