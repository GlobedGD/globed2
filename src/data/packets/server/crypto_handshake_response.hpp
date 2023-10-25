#pragma once
#include <data/packets/packet.hpp>
#include <util/data.hpp>
#include <crypto/box.hpp> // for crypto_box_PUBLICKEYBYTES

class CryptoHandshakeResponsePacket : public Packet {
    GLOBED_PACKET(20001, false)

    GLOBED_ENCODE_UNIMPL

    void decode(ByteBuffer& buf) override {
        serverkey = buf.readBytes(crypto_box_PUBLICKEYBYTES);
    }

    CryptoHandshakeResponsePacket(util::data::bytevector _serverkey) : serverkey(_serverkey) {}
    CryptoHandshakeResponsePacket() {}

    static CryptoHandshakeResponsePacket* create(util::data::bytevector serverkey) {
        return new CryptoHandshakeResponsePacket(serverkey);
    }

    util::data::bytevector serverkey;
};