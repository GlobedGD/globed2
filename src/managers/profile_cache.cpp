#include "profile_cache.hpp"

ProfileCacheManager::ProfileCacheManager() {}

void ProfileCacheManager::insert(PlayerAccountData data) {
    (*cache.lock())[data.id] = data;
}

std::optional<PlayerAccountData> ProfileCacheManager::getData(int32_t accountId) {
    auto cache_ = cache.lock();
    if (cache_->contains(accountId)) {
        return cache_->at(accountId);
    }

    return std::nullopt;
}

void ProfileCacheManager::clear() {
    cache.lock()->clear();
}