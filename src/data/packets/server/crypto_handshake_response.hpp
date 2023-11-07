#pragma once
#include <data/packets/packet.hpp>
#include <data/types/handshake.hpp>

class CryptoHandshakeResponsePacket : public Packet {
    GLOBED_PACKET(20001, false)

    GLOBED_PACKET_ENCODE_UNIMPL

    GLOBED_PACKET_DECODE {
        data = buf.readValue<HandshakeResponseData>();
    }

    HandshakeResponseData data;
};