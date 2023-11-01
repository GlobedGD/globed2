#pragma once
#include <data/packets/packet.hpp>
#include <data/connection.hpp>

class CryptoHandshakeResponsePacket : public Packet {
    GLOBED_PACKET(20001, false)

    GLOBED_ENCODE_UNIMPL

    GLOBED_DECODE {
        data = buf.readValue<HandshakeResponseData>();
    }

    HandshakeResponseData data;
};