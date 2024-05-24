#pragma once
#include <data/packets/packet.hpp>
#include <data/types/gd.hpp>
#include <data/types/room.hpp>

class RoomCreatedPacket : public Packet {
    GLOBED_PACKET(23000, false, false)

    RoomCreatedPacket() {}

    RoomInfo info;
};

GLOBED_SERIALIZABLE_STRUCT(RoomCreatedPacket, (info));

class RoomJoinedPacket : public Packet {
    GLOBED_PACKET(23001, false, false)

    RoomJoinedPacket() {}
};

GLOBED_SERIALIZABLE_STRUCT(RoomJoinedPacket, ());

class RoomJoinFailedPacket : public Packet {
    GLOBED_PACKET(23002, false, false)

    RoomJoinFailedPacket() {}

    bool wasInvalid, wasProtected, wasFull;
};

GLOBED_SERIALIZABLE_STRUCT(RoomJoinFailedPacket, (wasInvalid, wasProtected, wasFull));

class RoomPlayerListPacket : public Packet {
    GLOBED_PACKET(23003, false, false)

    RoomPlayerListPacket() {}

    RoomInfo info;
    std::vector<PlayerRoomPreviewAccountData> players;
};

GLOBED_SERIALIZABLE_STRUCT(RoomPlayerListPacket, (info, players));

class RoomInfoPacket : public Packet {
    GLOBED_PACKET(23004, false, false)

    RoomInfoPacket() {}

    RoomInfo info;
};

GLOBED_SERIALIZABLE_STRUCT(RoomInfoPacket, (info));

class RoomInvitePacket : public Packet {
    GLOBED_PACKET(23005, false, false)

    RoomInvitePacket() {}

    PlayerPreviewAccountData playerData;
    uint32_t roomID;
    std::string password;
};

GLOBED_SERIALIZABLE_STRUCT(RoomInvitePacket, (playerData, roomID, password));

class RoomListPacket : public Packet {
    GLOBED_PACKET(23006, false, false)

    RoomListPacket() {}

    std::vector<RoomListingInfo> rooms;
};

GLOBED_SERIALIZABLE_STRUCT(RoomListPacket, (rooms));
