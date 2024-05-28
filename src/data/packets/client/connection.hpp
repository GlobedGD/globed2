#pragma once
#include <data/packets/packet.hpp>
#include <data/types/crypto.hpp>
#include <data/types/gd.hpp>

class PingPacket : public Packet {
    GLOBED_PACKET(10000, PingPacket, false, false)

    PingPacket() {}
    PingPacket(uint32_t _id) : id(_id) {}

    uint32_t id;
};

GLOBED_SERIALIZABLE_STRUCT(PingPacket, (id));

class CryptoHandshakeStartPacket : public Packet {
    GLOBED_PACKET(10001, CryptoHandshakeStartPacket, false, true)

    CryptoHandshakeStartPacket() {}
    CryptoHandshakeStartPacket(uint16_t _protocol, CryptoPublicKey _key) : protocol(_protocol), key(_key) {}

    uint16_t protocol;
    CryptoPublicKey key;
};

GLOBED_SERIALIZABLE_STRUCT(CryptoHandshakeStartPacket, (protocol, key));

class KeepalivePacket : public Packet {
    GLOBED_PACKET(10002, KeepalivePacket, false, false)

    KeepalivePacket() {}
};

GLOBED_SERIALIZABLE_STRUCT(KeepalivePacket, ());

class LoginPacket : public Packet {
    GLOBED_PACKET(10003, LoginPacket, true, true)

    LoginPacket() {}
    LoginPacket(
            uint32_t secretKey,
            int32_t accid,
            int32_t userId,
            const std::string_view name,
            const std::string_view token,
            const PlayerIconData& icons,
            uint16_t fragmentationLimit,
            const std::string_view platform
    ) :
            secretKey(secretKey),
            accountId(accid),
            userId(userId),
            name(name),
            token(token),
            icons(icons),
            fragmentationLimit(fragmentationLimit),
            platform(platform) {}

    uint32_t secretKey;
    int32_t accountId;
    int32_t userId;
    std::string name;
    std::string token;
    PlayerIconData icons;
    uint16_t fragmentationLimit;
    std::string platform;
};

GLOBED_SERIALIZABLE_STRUCT(LoginPacket, (
    secretKey,
    accountId,
    userId,
    name,
    token,
    icons,
    fragmentationLimit,
    platform
));

class DisconnectPacket : public Packet {
    GLOBED_PACKET(10004, DisconnectPacket, false, false)

    DisconnectPacket() {}
};

GLOBED_SERIALIZABLE_STRUCT(DisconnectPacket, ());

class ClaimThreadPacket : public Packet {
    GLOBED_PACKET(10005, ClaimThreadPacket, false, false)

    ClaimThreadPacket() {}
    ClaimThreadPacket(uint32_t secretKey) : secretKey(secretKey) {}

    uint32_t secretKey;
};

GLOBED_SERIALIZABLE_STRUCT(ClaimThreadPacket, (secretKey));

class KeepaliveTCPPacket : public Packet {
    GLOBED_PACKET(10006, KeepaliveTCPPacket, false, true)

    KeepaliveTCPPacket() {}
};

GLOBED_SERIALIZABLE_STRUCT(KeepaliveTCPPacket, ());

class ConnectionTestPacket : public Packet {
    GLOBED_PACKET(10010, ConnectionTestPacket, false, false)

    ConnectionTestPacket() {}
    ConnectionTestPacket(uint32_t uid, util::data::bytevector&& vec) : uid(uid), data(std::move(vec)) {}

    uint32_t uid;
    util::data::bytevector data;
};

GLOBED_SERIALIZABLE_STRUCT(ConnectionTestPacket, (uid, data));
