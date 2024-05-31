#pragma once
#include <data/packets/packet.hpp>
#include <data/types/crypto.hpp>
#include <data/types/gd.hpp>
#include <data/types/user.hpp>

// 20000 - PingResponsePacket
class PingResponsePacket : public Packet {
    GLOBED_PACKET(20000, PingResponsePacket, false, false)

    PingResponsePacket() {}

    uint32_t id, playerCount;
};
GLOBED_SERIALIZABLE_STRUCT(PingResponsePacket, (id, playerCount));

// 20001 - CryptoHandshakeResponsePacket
class CryptoHandshakeResponsePacket : public Packet {
    GLOBED_PACKET(20001, CryptoHandshakeResponsePacket, false, false)

    CryptoHandshakeResponsePacket() {}

    CryptoPublicKey data;
};
GLOBED_SERIALIZABLE_STRUCT(CryptoHandshakeResponsePacket, (data));

// 20002 - KeepaliveResponsePacket
class KeepaliveResponsePacket : public Packet {
    GLOBED_PACKET(20002, KeepaliveResponsePacket, false, false)

    KeepaliveResponsePacket() {}

    uint32_t playerCount;
};
GLOBED_SERIALIZABLE_STRUCT(KeepaliveResponsePacket, (playerCount));

// 20003 - ServerDisconnectPacket
class ServerDisconnectPacket : public Packet {
    GLOBED_PACKET(20003, ServerDisconnectPacket, false, false)

    ServerDisconnectPacket() {}

    std::string message;
};
GLOBED_SERIALIZABLE_STRUCT(ServerDisconnectPacket, (message));

// 20004 - LoggedInPacket
class LoggedInPacket : public Packet {
    GLOBED_PACKET(20004, LoggedInPacket, false, false)

    LoggedInPacket() {}

    uint32_t tps;
    SpecialUserData specialUserData;
    std::vector<GameServerRole> allRoles;
    uint32_t secretKey;
};
GLOBED_SERIALIZABLE_STRUCT(LoggedInPacket, (tps, specialUserData, allRoles, secretKey));

// 20005 - LoginFailedPacket
class LoginFailedPacket : public Packet {
    GLOBED_PACKET(20005, LoginFailedPacket, false, false)

    LoginFailedPacket() {}

    std::string message;
};
GLOBED_SERIALIZABLE_STRUCT(LoginFailedPacket, (message));

// 20006 - ProtocolMismatchPacket
class ProtocolMismatchPacket : public Packet {
    GLOBED_PACKET(20006, ProtocolMismatchPacket, false, false)

    ProtocolMismatchPacket() {}

    uint16_t serverProtocol;
    std::string minClientVersion;
};
GLOBED_SERIALIZABLE_STRUCT(ProtocolMismatchPacket, (serverProtocol, minClientVersion));

// 20007 - KeepaliveTCPResponsePacket
class KeepaliveTCPResponsePacket : public Packet {
    GLOBED_PACKET(20007, KeepaliveTCPResponsePacket, false, false)

    KeepaliveTCPResponsePacket() {}
};
GLOBED_SERIALIZABLE_STRUCT(KeepaliveTCPResponsePacket, ());

// 20008 - ClaimThreadFailedPacket
class ClaimThreadFailedPacket : public Packet {
    GLOBED_PACKET(20008, ClaimThreadFailedPacket, false, false)

    ClaimThreadFailedPacket() {}
};
GLOBED_SERIALIZABLE_STRUCT(ClaimThreadFailedPacket, ());

// 20009 - LoginRecoveryFailecPacket
class LoginRecoveryFailecPacket : public Packet {
    GLOBED_PACKET(20009, LoginRecoveryFailecPacket, false, false)

    LoginRecoveryFailecPacket() {}
};
GLOBED_SERIALIZABLE_STRUCT(LoginRecoveryFailecPacket, ());

// 20100 - ServerNoticePacket
class ServerNoticePacket : public Packet {
    GLOBED_PACKET(20100, ServerNoticePacket, false, false)

    ServerNoticePacket() {}

    std::string message;
};
GLOBED_SERIALIZABLE_STRUCT(ServerNoticePacket, (message));

// 20101 - ServerBannedPacket
class ServerBannedPacket : public Packet {
    GLOBED_PACKET(20101, ServerBannedPacket, false, false)

    ServerBannedPacket() {}

    std::string message;
    int64_t timestamp;
};
GLOBED_SERIALIZABLE_STRUCT(ServerBannedPacket, (message, timestamp));

// 20102 - ServerMutedPacket
class ServerMutedPacket : public Packet {
    GLOBED_PACKET(20102, ServerMutedPacket, false, false)

    ServerMutedPacket() {}

    std::string reason;
    int64_t timestamp;
};
GLOBED_SERIALIZABLE_STRUCT(ServerMutedPacket, (reason, timestamp));

// 20200 - ConnectionTestResponsePacket
class ConnectionTestResponsePacket : public Packet {
    GLOBED_PACKET(20200, ConnectionTestResponsePacket, false, false)

    ConnectionTestResponsePacket() {}

    uint32_t uid;
    util::data::bytevector data;
};
GLOBED_SERIALIZABLE_STRUCT(ConnectionTestResponsePacket, (uid, data));
