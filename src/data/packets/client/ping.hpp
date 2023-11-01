#pragma once
#include <data/packets/packet.hpp>

class PingPacket : public Packet {
    GLOBED_PACKET(10000, false)

    GLOBED_ENCODE {
        buf.write(id);
    }

    GLOBED_DECODE_UNIMPL

    PingPacket(uint32_t _id) : id(_id) {}

    static PingPacket* create(uint32_t id) {
        return new PingPacket(id);
    }

    uint32_t id;
};