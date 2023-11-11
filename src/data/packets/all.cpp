#include "all.hpp"

#define PACKET(id, pt) case id: return std::make_shared<pt>()

std::shared_ptr<Packet> matchPacket(packetid_t packetId) {
    switch (packetId) {
        /*
        * Server-side packets
        */

        PACKET(20000, PingResponsePacket);
        PACKET(20001, CryptoHandshakeResponsePacket);
        PACKET(20002, ServerDisconnectPacket);
#if GLOBED_VOICE_SUPPORT
        PACKET(21010, VoiceBroadcastPacket);
#endif

        default:
            return std::shared_ptr<Packet>(nullptr);
    }
}