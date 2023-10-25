#pragma once
#include <data/packets/packet.hpp>
#include <util/data.hpp>

class CryptoHandshakeStartPacket : public Packet {
    GLOBED_PACKET(10001, false)

    void encode(ByteBuffer& buf) override {
        buf.writeBytes(pubkey);
    }

    GLOBED_DECODE_UNIMPL

    CryptoHandshakeStartPacket(util::data::bytevector _pubkey) : pubkey(_pubkey) {}
    CryptoHandshakeStartPacket() {}

    static CryptoHandshakeStartPacket* create(util::data::bytevector pubkey) {
        return new CryptoHandshakeStartPacket(pubkey);
    }

    util::data::bytevector pubkey;
};