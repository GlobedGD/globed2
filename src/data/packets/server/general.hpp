#pragma once
#include <data/packets/packet.hpp>
#include <data/types/gd.hpp>

class GlobalPlayerListPacket : public Packet {
    GLOBED_PACKET(21000, false)

    GLOBED_PACKET_DECODE { buf.readValueVectorInto<PlayerPreviewAccountData>(data); }

    std::vector<PlayerPreviewAccountData> data;
};

class RoomCreatedPacket : public Packet {
    GLOBED_PACKET(21001, false)

    GLOBED_PACKET_DECODE { roomId = buf.readU32(); }

    uint32_t roomId;
};

class RoomJoinedPacket : public Packet {
    GLOBED_PACKET(21002, false)
    GLOBED_PACKET_DECODE {}
};

class RoomJoinFailedPacket : public Packet {
    GLOBED_PACKET(21003, false)
    GLOBED_PACKET_DECODE {}
};

class RoomPlayerListPacket : public Packet {
    GLOBED_PACKET(21004, false)

    GLOBED_PACKET_DECODE {
        roomId = buf.readU32();
        buf.readValueVectorInto<PlayerRoomPreviewAccountData>(data);
    }

    uint32_t roomId;
    std::vector<PlayerRoomPreviewAccountData> data;
};