#pragma once
#include <data/packets/packet.hpp>
#include <data/types/crypto.hpp>
#include <data/types/gd.hpp>

class PingResponsePacket : public Packet {
    GLOBED_PACKET(20000, false, false)

    GLOBED_PACKET_DECODE {
        id = buf.readU32();
        playerCount = buf.readU32();
    }

    uint32_t id, playerCount;
};

class CryptoHandshakeResponsePacket : public Packet {
    GLOBED_PACKET(20001, false, false)

    GLOBED_PACKET_DECODE { data = buf.readValue<CryptoPublicKey>(); }

    CryptoPublicKey data;
};

class KeepaliveResponsePacket : public Packet {
    GLOBED_PACKET(20002, false, false)

    GLOBED_PACKET_DECODE { playerCount = buf.readU32(); }

    uint32_t playerCount;
};

class ServerDisconnectPacket : public Packet {
    GLOBED_PACKET(20003, false, false)

    GLOBED_PACKET_DECODE { message = buf.readString(); }

    std::string message;
};

class LoggedInPacket : public Packet {
    GLOBED_PACKET(20004, false, false)

    GLOBED_PACKET_DECODE {
        tps = buf.readU32();
        specialUserData = buf.readOptionalValue<SpecialUserData>();
    }

    uint32_t tps;
    std::optional<SpecialUserData> specialUserData;
};

class LoginFailedPacket : public Packet {
    GLOBED_PACKET(20005, false, false)

    GLOBED_PACKET_DECODE { message = buf.readString(); }

    std::string message;
};

class ServerNoticePacket : public Packet {
    GLOBED_PACKET(20006, false, false)

    GLOBED_PACKET_DECODE { message = buf.readString(); }

    std::string message;
};

class ProtocolMismatchPacket : public Packet {
    GLOBED_PACKET(20007, false, false)

    GLOBED_PACKET_DECODE { serverProtocol = buf.readU16(); }

    uint16_t serverProtocol;
};

class KeepaliveTCPResponsePacket : public Packet {
    GLOBED_PACKET(20008, false, false)

    GLOBED_PACKET_DECODE {}
};

class ConnectionTestResponsePacket : public Packet {
    GLOBED_PACKET(20010, false, false)

    GLOBED_PACKET_DECODE {
        uid = buf.readU32();
        data = buf.readByteArray();
    }

    uint32_t uid;
    util::data::bytevector data;
};
