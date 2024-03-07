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

class RoomCreatedPacket : public Packet {
    GLOBED_PACKET(21001, false, false)

    RoomCreatedPacket() {}

    uint32_t roomId;
};

GLOBED_SERIALIZABLE_STRUCT(RoomCreatedPacket, (roomId));

class RoomJoinedPacket : public Packet {
    GLOBED_PACKET(21002, false, false)

    RoomJoinedPacket() {}
};

GLOBED_SERIALIZABLE_STRUCT(RoomJoinedPacket, ());

class RoomJoinFailedPacket : public Packet {
    GLOBED_PACKET(21003, false, false)

    RoomJoinFailedPacket() {}
};

GLOBED_SERIALIZABLE_STRUCT(RoomJoinFailedPacket, ());

class RoomPlayerListPacket : public Packet {
    GLOBED_PACKET(21004, false, false)

    RoomPlayerListPacket() {}

    uint32_t roomId;
    std::vector<PlayerRoomPreviewAccountData> data;
};

GLOBED_SERIALIZABLE_STRUCT(RoomPlayerListPacket, (roomId, data));

class LevelListPacket : public Packet {
    GLOBED_PACKET(21005, false, false)

    LevelListPacket() {}

    std::vector<GlobedLevel> levels;
};

GLOBED_SERIALIZABLE_STRUCT(LevelListPacket, (levels));

class LevelPlayerCountPacket : public Packet {
    GLOBED_PACKET(21006, false, false)

    LevelPlayerCountPacket() {}

    std::vector<std::pair<LevelId, uint16_t>> levels;
};

GLOBED_SERIALIZABLE_STRUCT(LevelPlayerCountPacket, (levels));
