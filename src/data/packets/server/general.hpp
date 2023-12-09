#pragma once
#include <data/packets/packet.hpp>
#include <data/types/gd.hpp>

class PlayerListPacket : public Packet {
    GLOBED_PACKET(21000, false)

    GLOBED_PACKET_ENCODE_UNIMPL
    GLOBED_PACKET_DECODE { data = buf.readValueVector<PlayerPreviewAccountData>(); }

    std::vector<PlayerPreviewAccountData> data;
};

class RoomCreatedPacket : public Packet {
    GLOBED_PACKET(21001, false)

    GLOBED_PACKET_ENCODE_UNIMPL
    GLOBED_PACKET_DECODE { roomId = buf.readString(); }

    std::string roomId;
};

class RoomJoinedPacket : public Packet {
    GLOBED_PACKET(21002, false)
    GLOBED_PACKET_ENCODE_UNIMPL
    GLOBED_PACKET_DECODE {}
};

class RoomJoinFailedPacket : public Packet {
    GLOBED_PACKET(21003, false)
    GLOBED_PACKET_ENCODE_UNIMPL
    GLOBED_PACKET_DECODE {}
};
