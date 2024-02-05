#pragma once
#include <defs.hpp>

class BlockListMangaer : public SingletonBase<BlockListMangaer> {
protected:
    friend class SingletonBase;
    BlockListMangaer();

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
