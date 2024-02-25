#pragma once
#include <data/bytebuffer.hpp>
#include "gd.hpp"

class GameServerEntry {
public:
    GLOBED_ENCODE {
        buf.writeString(id);
        buf.writeString(name);
        buf.writeString(address);
        buf.writeString(region);
    }

    GLOBED_DECODE {
        id = buf.readString();
        name = buf.readString();
        address = buf.readString();
        region = buf.readString();
    }

    std::string id, name, address, region;
};

class GlobedLevel {
public:
    GLOBED_ENCODE {
        buf.writePrimitive(levelId);
        buf.writeU16(playerCount);
    }

    GLOBED_DECODE {
        levelId = buf.readPrimitive<LevelId>();
        playerCount = buf.readU16();
    }

    LevelId levelId;
    unsigned short playerCount;
};
