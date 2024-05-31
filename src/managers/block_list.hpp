#pragma once
#include <defs/minimal_geode.hpp>

#include <util/singleton.hpp>

class BlockListManager : public SingletonBase<BlockListManager> {
protected:
    friend class SingletonBase;
    BlockListManager();

    static constexpr std::string_view SETTING_KEY = "_globed-blocklist-manager-vals";

public:
    bool isExplicitlyBlocked(int playerId);
    bool isExplicitlyAllowed(int playerId);

    void blacklist(int playerId);
    void whitelist(int playerId);

    bool isHidden(int playerId);
    void setHidden(int playerId, bool state);

private:
    std::set<int> _bl, _wl;
    std::set<int> hiddenPlayers;

    void save();
    Result<> load();
};
