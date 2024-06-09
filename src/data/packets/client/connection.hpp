#pragma once
#include <data/packets/packet.hpp>
#include <data/types/crypto.hpp>
#include <data/types/gd.hpp>

// 10000 - PingPacket
class PingPacket : public Packet {
    GLOBED_PACKET(10000, PingPacket, false, false)

    PingPacket() {}
    PingPacket(uint32_t _id) : id(_id) {}

    uint32_t id;
};

GLOBED_SERIALIZABLE_STRUCT(PingPacket, (id));

// 10001 - CryptoHandshakeStartPacket
class CryptoHandshakeStartPacket : public Packet {
    GLOBED_PACKET(10001, CryptoHandshakeStartPacket, false, true)

    CryptoHandshakeStartPacket() {}
    CryptoHandshakeStartPacket(uint16_t _protocol, CryptoPublicKey _key) : protocol(_protocol), key(_key) {}

    uint16_t protocol;
    CryptoPublicKey key;
};

GLOBED_SERIALIZABLE_STRUCT(CryptoHandshakeStartPacket, (protocol, key));

// 10002 - KeepalivePacket
class KeepalivePacket : public Packet {
    GLOBED_PACKET(10002, KeepalivePacket, false, false)

    KeepalivePacket() {}
};

GLOBED_SERIALIZABLE_STRUCT(KeepalivePacket, ());

// 10003 - LoginPacket
class LoginPacket : public Packet {
    GLOBED_PACKET(10003, LoginPacket, true, true)

    LoginPacket() {}
    LoginPacket(
            int32_t accid,
            int32_t userId,
            const std::string_view name,
            const std::string_view token,
            const PlayerIconData& icons,
            uint16_t fragmentationLimit,
            const std::string_view platform
    ) :
            accountId(accid),
            userId(userId),
            name(name),
            token(token),
            icons(icons),
            fragmentationLimit(fragmentationLimit),
            platform(platform) {}

    int32_t accountId;
    int32_t userId;
    std::string name;
    std::string token;
    PlayerIconData icons;
    uint16_t fragmentationLimit;
    std::string platform;
};

GLOBED_SERIALIZABLE_STRUCT(LoginPacket, (
    accountId,
    userId,
    name,
    token,
    icons,
    fragmentationLimit,
    platform
));

// 10005 - ClaimThreadPacket
class ClaimThreadPacket : public Packet {
    GLOBED_PACKET(10005, ClaimThreadPacket, false, false)

    ClaimThreadPacket() {}
    ClaimThreadPacket(uint32_t secretKey) : secretKey(secretKey) {}

    uint32_t secretKey;
};

// 10006 - DisconnectPacket
class DisconnectPacket : public Packet {
    GLOBED_PACKET(10006, DisconnectPacket, false, false)

    DisconnectPacket() {}
};

GLOBED_SERIALIZABLE_STRUCT(DisconnectPacket, ());

GLOBED_SERIALIZABLE_STRUCT(ClaimThreadPacket, (secretKey));

// 10007 - KeepaliveTCPPacket
class KeepaliveTCPPacket : public Packet {
    GLOBED_PACKET(10007, KeepaliveTCPPacket, false, true)

    KeepaliveTCPPacket() {}
};

GLOBED_SERIALIZABLE_STRUCT(KeepaliveTCPPacket, ());

// 10200 - ConnectionTestPacket
class ConnectionTestPacket : public Packet {
    GLOBED_PACKET(10200, ConnectionTestPacket, false, false)

    ConnectionTestPacket() {}
    ConnectionTestPacket(uint32_t uid, util::data::bytevector&& vec) : uid(uid), data(std::move(vec)) {}

    uint32_t uid;
    util::data::bytevector data;
};

GLOBED_SERIALIZABLE_STRUCT(ConnectionTestPacket, (uid, data));
