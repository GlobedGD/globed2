#pragma once
#include <data/bytebuffer.hpp>
#include "gd.hpp"

class GameServerEntry {
public:
    std::string id, name, address, region;
};

GLOBED_SERIALIZABLE_STRUCT(GameServerEntry, (
    id, name, address, region
));

class GlobedLevel {
public:
    LevelId levelId;
    unsigned short playerCount;
};

GLOBED_SERIALIZABLE_STRUCT(GlobedLevel, (
    levelId, playerCount
));

