#pragma once
#include <data/bytebuffer.hpp>

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
        buf.writeI32(levelId);
        buf.writeU16(playerCount);
    }

    GLOBED_DECODE {
        levelId = buf.readI32();
        playerCount = buf.readU16();
    }

    int levelId;
    unsigned short playerCount;
};
