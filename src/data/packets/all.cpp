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
        PACKET(LoginFailedPacket);
        PACKET(ServerNoticePacket);

        // general

        PACKET(GlobalPlayerListPacket);
        PACKET(RoomCreatedPacket);
        PACKET(RoomJoinedPacket);
        PACKET(RoomJoinFailedPacket);
        PACKET(RoomPlayerListPacket);

        // game related

        PACKET(PlayerProfilesPacket);
        PACKET(LevelDataPacket);
#if GLOBED_VOICE_SUPPORT
        PACKET(VoiceBroadcastPacket);
#endif
        PACKET(ChatMessageBroadcastPacket);

        // admin related

        PACKET(AdminAuthSuccessPacket);

        default:
            return std::shared_ptr<Packet>(nullptr);
    }
}