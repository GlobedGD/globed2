#include "all.hpp"

#define PACKET(id, pt) case id: return std::make_unique<pt>()

std::unique_ptr<Packet> matchPacket(packetid_t packetId) {
    switch (packetId) {
        /*
        * Client-side packets
        */

        PACKET(10000, PingPacket);
        PACKET(10001, CryptoHandshakeStartPacket);

        /*
        * Server-side packets
        */

        PACKET(20000, PingResponsePacket);
        PACKET(20001, CryptoHandshakeResponsePacket);

        default:
            return std::unique_ptr<Packet>(nullptr);
    }
}