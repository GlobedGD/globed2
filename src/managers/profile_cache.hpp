#pragma once
#include <defs.hpp>
#include <data/types/gd.hpp>
#include <util/sync.hpp>

// This class is fully thread safe to use.
class ProfileCacheManager : GLOBED_SINGLETON(ProfileCacheManager) {
public:
    ProfileCacheManager();

    void insert(PlayerAccountData data);
    std::optional<PlayerAccountData> getData(int32_t accountId);
    void clear();

private:
    util::sync::WrappingMutex<std::unordered_map<int32_t, PlayerAccountData>> cache;
};