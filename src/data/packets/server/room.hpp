#pragma once
#include <data/packets/packet.hpp>
#include <data/types/gd.hpp>
#include <data/types/room.hpp>

// 23000 - RoomCreatedPacket
class RoomCreatedPacket : public Packet {
    GLOBED_PACKET(23000, RoomCreatedPacket, false, false)

    RoomCreatedPacket() {}

    RoomInfo info;
};

GLOBED_SERIALIZABLE_STRUCT(RoomCreatedPacket, (info));

// 23001 - RoomJoinedPacket
class RoomJoinedPacket : public Packet {
    GLOBED_PACKET(23001, RoomJoinedPacket, false, false)

    RoomJoinedPacket() {}
};

GLOBED_SERIALIZABLE_STRUCT(RoomJoinedPacket, ());

// 23002 - RoomJoinFailedPacket
class RoomJoinFailedPacket : public Packet {
    GLOBED_PACKET(23002, RoomJoinFailedPacket, false, false)

    RoomJoinFailedPacket() {}

    bool wasInvalid, wasProtected, wasFull;
};

GLOBED_SERIALIZABLE_STRUCT(RoomJoinFailedPacket, (wasInvalid, wasProtected, wasFull));

// 23003 - RoomPlayerListPacket
class RoomPlayerListPacket : public Packet {
    GLOBED_PACKET(23003, RoomPlayerListPacket, false, false)

    RoomPlayerListPacket() {}

    RoomInfo info;
    std::vector<PlayerRoomPreviewAccountData> players;
};

GLOBED_SERIALIZABLE_STRUCT(RoomPlayerListPacket, (info, players));

// 23004 - RoomInfoPacket
class RoomInfoPacket : public Packet {
    GLOBED_PACKET(23004, RoomInfoPacket, false, false)

    RoomInfoPacket() {}

    RoomInfo info;
};

GLOBED_SERIALIZABLE_STRUCT(RoomInfoPacket, (info));

// 23005 - RoomInvitePacket
class RoomInvitePacket : public Packet {
    GLOBED_PACKET(23005, RoomInvitePacket, false, false)

    RoomInvitePacket() {}

    PlayerPreviewAccountData playerData;
    uint32_t roomID;
    std::string password;
};

GLOBED_SERIALIZABLE_STRUCT(RoomInvitePacket, (playerData, roomID, password));

// 23006 - RoomListPacket
class RoomListPacket : public Packet {
    GLOBED_PACKET(23006, RoomListPacket, false, false)

    RoomListPacket() {}

    std::vector<RoomListingInfo> rooms;
};

GLOBED_SERIALIZABLE_STRUCT(RoomListPacket, (rooms));
