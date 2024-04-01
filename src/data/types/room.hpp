#pragma once
#include <data/bytebuffer.hpp>

class RoomSettings {
public:
    bool collision;

    uint64_t reserved;
};

class RoomInfo {
public:
    uint32_t id;
    int owner;
    RoomSettings settings;
};

GLOBED_SERIALIZABLE_STRUCT(RoomInfo, (
    id, owner, settings
));
