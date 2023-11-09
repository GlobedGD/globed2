#pragma once
#include <data/packets/packet.hpp>
#include <data/types/handshake.hpp>

class PingPacket : public Packet {
    GLOBED_PACKET(10000, false)

    GLOBED_PACKET_ENCODE {
        buf.writeU32(id);
    }

    GLOBED_PACKET_DECODE_UNIMPL

    PingPacket(uint32_t _id) : id(_id) {}

    static PingPacket* create(uint32_t id) {
        return new PingPacket(id);
    }

    uint32_t id;
};

class CryptoHandshakeStartPacket : public Packet {
    GLOBED_PACKET(10001, false)

    GLOBED_PACKET_ENCODE {
        buf.writeValue(data);
    }

    GLOBED_PACKET_DECODE_UNIMPL

    CryptoHandshakeStartPacket(HandshakeData _data) : data(_data) {}

    static CryptoHandshakeStartPacket* create(HandshakeData data) {
        return new CryptoHandshakeStartPacket(data);
    }

    HandshakeData data;
};