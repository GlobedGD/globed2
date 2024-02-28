#pragma once
#include <defs/util.hpp>
#include <data/types/gd.hpp>
#include <util/sync.hpp>

class ProfileCacheManager : public SingletonBase<ProfileCacheManager> {
public:
    void insert(const PlayerAccountData& data);
    std::optional<PlayerAccountData> getData(int32_t accountId);
    void clear();

    // gather player's icons and call `setOwnData`;
    void setOwnDataAuto();
    void setOwnData(const PlayerIconData& data);
    void setOwnSpecialData(const std::optional<SpecialUserData>& specialUserData);
    PlayerIconData getOwnData();
    PlayerAccountData& getOwnAccountData();
    std::optional<SpecialUserData>& getOwnSpecialData();

    bool pendingChanges = false;

private:
    std::unordered_map<int32_t, PlayerAccountData> cache;
    PlayerAccountData ownData;
    std::optional<SpecialUserData> ownSpecialData;
};