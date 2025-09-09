#pragma once

#include <globed/util/singleton.hpp>
#include <globed/core/data/PlayerDisplayData.hpp>
#include <globed/core/data/SpecialUserData.hpp>

namespace globed {

class PlayerCacheManager : public SingletonBase<PlayerCacheManager> {
public:
    using SingletonBase::get;

    void insert(int accountId, const PlayerDisplayData& data);
    void remove(int accountId);

    std::optional<PlayerDisplayData> get(int accountId);
    bool has(int accountId);
    bool hasInLayer1(int accountId);

    void evictToLayer2(int accountId);
    void evictAllToLayer2();

private:
    friend class SingletonBase;
    struct Entry {
        PlayerDisplayData data;
    };

    // Layer 1 is the primary and accurate cache, layer 2 is for players that may have changed their data
    std::unordered_map<int, Entry> m_layer1, m_layer2;

    PlayerCacheManager() = default;
};

}