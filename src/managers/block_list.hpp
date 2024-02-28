#pragma once
#include <defs/util.hpp>

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

private:
    std::set<int> _bl, _wl;

    void save();
    void load();
};
