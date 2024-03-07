#pragma once
#include <data/packets/packet.hpp>
#include <data/types/crypto.hpp>
#include <data/types/gd.hpp>

class PingResponsePacket : public Packet {
    GLOBED_PACKET(20000, false, false)

    PingResponsePacket() {}

    uint32_t id, playerCount;
};

GLOBED_SERIALIZABLE_STRUCT(PingResponsePacket, (id, playerCount));

class CryptoHandshakeResponsePacket : public Packet {
    GLOBED_PACKET(20001, false, false)

    CryptoHandshakeResponsePacket() {}

    CryptoPublicKey data;
};

GLOBED_SERIALIZABLE_STRUCT(CryptoHandshakeResponsePacket, (data));

class KeepaliveResponsePacket : public Packet {
    GLOBED_PACKET(20002, false, false)

    KeepaliveResponsePacket() {}

    uint32_t playerCount;
};

GLOBED_SERIALIZABLE_STRUCT(KeepaliveResponsePacket, (playerCount));

class ServerDisconnectPacket : public Packet {
    GLOBED_PACKET(20003, false, false)

    ServerDisconnectPacket() {}

    std::string message;
};

GLOBED_SERIALIZABLE_STRUCT(ServerDisconnectPacket, (message));

class LoggedInPacket : public Packet {
    GLOBED_PACKET(20004, false, false)

    LoggedInPacket() {}

    uint32_t tps;
    std::optional<SpecialUserData> specialUserData;
};

GLOBED_SERIALIZABLE_STRUCT(LoggedInPacket, (tps, specialUserData));

class LoginFailedPacket : public Packet {
    GLOBED_PACKET(20005, false, false)

    LoginFailedPacket() {}

    std::string message;
};

GLOBED_SERIALIZABLE_STRUCT(LoginFailedPacket, (message));

class ServerNoticePacket : public Packet {
    GLOBED_PACKET(20006, false, false)

    ServerNoticePacket() {}

    std::string message;
};

GLOBED_SERIALIZABLE_STRUCT(ServerNoticePacket, (message));

class ProtocolMismatchPacket : public Packet {
    GLOBED_PACKET(20007, false, false)

    ProtocolMismatchPacket() {}

    uint16_t serverProtocol;
};

GLOBED_SERIALIZABLE_STRUCT(ProtocolMismatchPacket, (serverProtocol));

class KeepaliveTCPResponsePacket : public Packet {
    GLOBED_PACKET(20008, false, false)

    KeepaliveTCPResponsePacket() {}
};

GLOBED_SERIALIZABLE_STRUCT(KeepaliveTCPResponsePacket, ());

class ConnectionTestResponsePacket : public Packet {
    GLOBED_PACKET(20010, false, false)

    ConnectionTestResponsePacket() {}

    uint32_t uid;
    util::data::bytevector data;
};

GLOBED_SERIALIZABLE_STRUCT(ConnectionTestResponsePacket, (uid, data));
