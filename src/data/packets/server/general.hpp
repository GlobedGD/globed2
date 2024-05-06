#pragma once
#include <data/packets/packet.hpp>
#include <data/types/gd.hpp>
#include <data/types/misc.hpp>
#include <data/types/room.hpp>

class GlobalPlayerListPacket : public Packet {
    GLOBED_PACKET(21000, false, false)

    GlobalPlayerListPacket() {}

    std::vector<PlayerPreviewAccountData> data;
};

GLOBED_SERIALIZABLE_STRUCT(GlobalPlayerListPacket, (data));

class RoomCreatedPacket : public Packet {
    GLOBED_PACKET(21001, false, false)

    RoomCreatedPacket() {}

    RoomInfo info;
};

GLOBED_SERIALIZABLE_STRUCT(RoomCreatedPacket, (info));

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

    RoomInfo info;
    std::vector<PlayerRoomPreviewAccountData> players;
};

GLOBED_SERIALIZABLE_STRUCT(RoomPlayerListPacket, (info, players));

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

class RoomInfoPacket : public Packet {
    GLOBED_PACKET(21007, false, false)

    RoomInfoPacket() {}

    RoomInfo info;
};

GLOBED_SERIALIZABLE_STRUCT(RoomInfoPacket, (info));

class RoomInvitePacket : public Packet {
    GLOBED_PACKET(21008, false, false)

    RoomInvitePacket() {}

    PlayerRoomPreviewAccountData playerData;
    uint32_t roomID;
};

GLOBED_SERIALIZABLE_STRUCT(RoomInvitePacket, (playerData, roomID));
