#pragma once
#include <data/packets/packet.hpp>
#include <data/types/gd.hpp>
#include <data/types/misc.hpp>

class GlobalPlayerListPacket : public Packet {
    GLOBED_PACKET(21000, false, false)

    GlobalPlayerListPacket() {}

    std::vector<PlayerPreviewAccountData> data;
};

GLOBED_SERIALIZABLE_STRUCT(GlobalPlayerListPacket, (data));

class LevelListPacket : public Packet {
    GLOBED_PACKET(21001, false, false)

    LevelListPacket() {}

    std::vector<GlobedLevel> levels;
};

GLOBED_SERIALIZABLE_STRUCT(LevelListPacket, (levels));

class LevelPlayerCountPacket : public Packet {
    GLOBED_PACKET(21002, false, false)

    LevelPlayerCountPacket() {}

    std::vector<std::pair<LevelId, uint16_t>> levels;
};

GLOBED_SERIALIZABLE_STRUCT(LevelPlayerCountPacket, (levels));
