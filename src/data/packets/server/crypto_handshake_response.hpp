#pragma once
#include <data/packets/packet.hpp>
#include <data/connection.hpp>

class CryptoHandshakeResponsePacket : public Packet {
    GLOBED_PACKET(20001, false)

    GLOBED_ENCODE_UNIMPL

    void decode(ByteBuffer& buf) override {
        data = buf.readValue<HandshakeResponseData>();
    }

    CryptoHandshakeResponsePacket(HandshakeResponseData _data) : data(_data) {}
    CryptoHandshakeResponsePacket() {}

    static CryptoHandshakeResponsePacket* create(HandshakeResponseData data) {
        return new CryptoHandshakeResponsePacket(data);
    }

    HandshakeResponseData data;
};