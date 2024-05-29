#pragma once
#include <data/packets/packet.hpp>
#include <data/types/crypto.hpp>
#include <data/types/gd.hpp>
#include <data/types/user.hpp>

class PingResponsePacket : public Packet {
    GLOBED_PACKET(20000, PingResponsePacket, false, false)

    PingResponsePacket() {}

    uint32_t id, playerCount;
};

GLOBED_SERIALIZABLE_STRUCT(PingResponsePacket, (id, playerCount));

class CryptoHandshakeResponsePacket : public Packet {
    GLOBED_PACKET(20001, CryptoHandshakeResponsePacket, false, false)

    CryptoHandshakeResponsePacket() {}

    CryptoPublicKey data;
};

GLOBED_SERIALIZABLE_STRUCT(CryptoHandshakeResponsePacket, (data));

class KeepaliveResponsePacket : public Packet {
    GLOBED_PACKET(20002, KeepaliveResponsePacket, false, false)

    KeepaliveResponsePacket() {}

    uint32_t playerCount;
};

GLOBED_SERIALIZABLE_STRUCT(KeepaliveResponsePacket, (playerCount));

class ServerDisconnectPacket : public Packet {
    GLOBED_PACKET(20003, ServerDisconnectPacket, false, false)

    ServerDisconnectPacket() {}

    std::string message;
};

GLOBED_SERIALIZABLE_STRUCT(ServerDisconnectPacket, (message));

class LoggedInPacket : public Packet {
    GLOBED_PACKET(20004, LoggedInPacket, false, false)

    LoggedInPacket() {}

    uint32_t tps;
    SpecialUserData specialUserData;
    std::vector<GameServerRole> allRoles;
    uint32_t secretKey;
};

GLOBED_SERIALIZABLE_STRUCT(LoggedInPacket, (tps, specialUserData, allRoles, secretKey));

class LoginFailedPacket : public Packet {
    GLOBED_PACKET(20005, LoginFailedPacket, false, false)

    LoginFailedPacket() {}

    std::string message;
};

GLOBED_SERIALIZABLE_STRUCT(LoginFailedPacket, (message));

class ServerNoticePacket : public Packet {
    GLOBED_PACKET(20006, ServerNoticePacket, false, false)

    ServerNoticePacket() {}

    std::string message;
};

GLOBED_SERIALIZABLE_STRUCT(ServerNoticePacket, (message));

class ProtocolMismatchPacket : public Packet {
    GLOBED_PACKET(20007, ProtocolMismatchPacket, false, false)

    ProtocolMismatchPacket() {}

    uint16_t serverProtocol;
};

GLOBED_SERIALIZABLE_STRUCT(ProtocolMismatchPacket, (serverProtocol));

class KeepaliveTCPResponsePacket : public Packet {
    GLOBED_PACKET(20008, KeepaliveTCPResponsePacket, false, false)

    KeepaliveTCPResponsePacket() {}
};

GLOBED_SERIALIZABLE_STRUCT(KeepaliveTCPResponsePacket, ());

class ConnectionTestResponsePacket : public Packet {
    GLOBED_PACKET(20010, ConnectionTestResponsePacket, false, false)

    ConnectionTestResponsePacket() {}

    uint32_t uid;
    util::data::bytevector data;
};

GLOBED_SERIALIZABLE_STRUCT(ConnectionTestResponsePacket, (uid, data));

class ServerBannedPacket : public Packet {
    GLOBED_PACKET(20011, ServerBannedPacket, false, false)

    ServerBannedPacket() {}

    std::string message;
    int64_t timestamp;
};

GLOBED_SERIALIZABLE_STRUCT(ServerBannedPacket, (message, timestamp))

class ServerMutedPacket : public Packet {
    GLOBED_PACKET(20012, ServerMutedPacket, false, false)

    ServerMutedPacket() {}

    std::string reason;
    int64_t timestamp;
};

GLOBED_SERIALIZABLE_STRUCT(ServerMutedPacket, (reason, timestamp));
