#include "all.hpp"

#define PACKET(pt) case pt::PACKET_ID: return std::make_shared<pt>()

std::shared_ptr<Packet> matchPacket(packetid_t packetId) {
    switch (packetId) {
        // connection related

        PACKET(PingResponsePacket);
        PACKET(CryptoHandshakeResponsePacket);
        PACKET(KeepaliveResponsePacket);
        PACKET(ServerDisconnectPacket);
        PACKET(LoggedInPacket);
        PACKET(ServerNoticePacket);

        // game related

        PACKET(PlayerProfilesPacket);
        PACKET(LevelDataPacket);
        PACKET(PlayerListPacket);
#if GLOBED_VOICE_SUPPORT
        PACKET(VoiceBroadcastPacket);
#endif

        default:
            return std::shared_ptr<Packet>(nullptr);
    }
}