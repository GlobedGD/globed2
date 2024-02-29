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
        PACKET(ProtocolMismatchPacket);
        PACKET(KeepaliveTCPResponsePacket);
        PACKET(ConnectionTestResponsePacket);

        // general

        PACKET(GlobalPlayerListPacket);
        PACKET(RoomCreatedPacket);
        PACKET(RoomJoinedPacket);
        PACKET(RoomJoinFailedPacket);
        PACKET(RoomPlayerListPacket);
        PACKET(LevelListPacket);
        PACKET(LevelPlayerCountPacket);

        // game related

        PACKET(PlayerProfilesPacket);
        PACKET(LevelDataPacket);
        PACKET(LevelPlayerMetadataPacket);
        PACKET(VoiceBroadcastPacket);
        PACKET(ChatMessageBroadcastPacket);

        // admin related

        PACKET(AdminAuthSuccessPacket);
        PACKET(AdminErrorPacket);
        PACKET(AdminUserDataPacket);
        PACKET(AdminSuccessMessagePacket);
        PACKET(AdminAuthFailedPacket);

        default:
            return std::shared_ptr<Packet>(nullptr);
    }
}