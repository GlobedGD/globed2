#pragma once
#include <data/packets/packet.hpp>

class PingResponsePacket : public Packet {
    GLOBED_PACKET(20000, false)

    GLOBED_ENCODE_UNIMPL

    GLOBED_DECODE {
        id = buf.readU32();
        playerCount = buf.readU32();
    }

    uint32_t id, playerCount;
};