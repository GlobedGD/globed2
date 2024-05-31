#pragma once
#include <asp/sync.hpp>

#include <defs/geode.hpp>
#include <data/types/gd.hpp>
#include <util/singleton.hpp>

class ProfileCacheManager : public SingletonBase<ProfileCacheManager> {
public:
    void insert(const PlayerAccountData& data);
    std::optional<PlayerAccountData> getData(int32_t accountId);
    void clear();

    // gather player's icons and call `setOwnData`;
    void setOwnDataAuto();
    void setOwnData(const PlayerIconData& data);
    void setOwnSpecialData(const SpecialUserData& specialUserData);
    PlayerIconData getOwnData();
    PlayerAccountData& getOwnAccountData();
    SpecialUserData& getOwnSpecialData();

    bool pendingChanges = false;

private:
    std::unordered_map<int32_t, PlayerAccountData> cache;
    PlayerAccountData ownData;
    SpecialUserData ownSpecialData;
};