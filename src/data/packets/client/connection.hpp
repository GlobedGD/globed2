#pragma once
#include <data/packets/packet.hpp>
#include <data/types/crypto.hpp>
#include <data/types/gd.hpp>

class PingPacket : public Packet {
    GLOBED_PACKET(10000, false, false)

    PingPacket() {}
    PingPacket(uint32_t _id) : id(_id) {}

    static std::shared_ptr<Packet> create(uint32_t id) {
        return std::make_shared<PingPacket>(id);
    }

    uint32_t id;
};

GLOBED_SERIALIZABLE_STRUCT(PingPacket, (id));

class CryptoHandshakeStartPacket : public Packet {
    GLOBED_PACKET(10001, false, true)

    CryptoHandshakeStartPacket() {}
    CryptoHandshakeStartPacket(uint16_t _protocol, CryptoPublicKey _key) : protocol(_protocol), key(_key) {}

    static std::shared_ptr<Packet> create(uint16_t protocol, CryptoPublicKey key) {
        return std::make_shared<CryptoHandshakeStartPacket>(protocol, key);
    }

    uint16_t protocol;
    CryptoPublicKey key;
};

GLOBED_SERIALIZABLE_STRUCT(CryptoHandshakeStartPacket, (protocol, key));

class KeepalivePacket : public Packet {
    GLOBED_PACKET(10002, false, false)

    KeepalivePacket() {}

    static std::shared_ptr<Packet> create() {
        return std::make_shared<KeepalivePacket>();
    }
};

GLOBED_SERIALIZABLE_STRUCT(KeepalivePacket, ());

class LoginPacket : public Packet {
    GLOBED_PACKET(10003, true, true)

    LoginPacket() {}
    LoginPacket(uint32_t secretKey, int32_t accid, int32_t userId, const std::string_view name, const std::string_view token, const PlayerIconData& icons, uint16_t fragmentationLimit)
        : secretKey(secretKey), accountId(accid), userId(userId), name(name), token(token), icons(icons), fragmentationLimit(fragmentationLimit) {}

    static std::shared_ptr<Packet> create(uint32_t secretKey, int32_t accid, int32_t userId, const std::string_view name, const std::string_view token, const PlayerIconData& icons, uint16_t fragmentationLimit) {
        return std::make_shared<LoginPacket>(secretKey, accid, userId, name, token, icons, fragmentationLimit);
    }

    uint32_t secretKey;
    int32_t accountId;
    int32_t userId;
    std::string name;
    std::string token;
    PlayerIconData icons;
    uint16_t fragmentationLimit;
};

GLOBED_SERIALIZABLE_STRUCT(LoginPacket, (
    secretKey,
    accountId,
    userId,
    name,
    token,
    icons,
    fragmentationLimit
));

class DisconnectPacket : public Packet {
    GLOBED_PACKET(10004, false, false)

    DisconnectPacket() {}

    static std::shared_ptr<Packet> create() {
        return std::make_shared<DisconnectPacket>();
    }
};

GLOBED_SERIALIZABLE_STRUCT(DisconnectPacket, ());

class ClaimThreadPacket : public Packet {
    GLOBED_PACKET(10005, false, false)

    ClaimThreadPacket() {}
    ClaimThreadPacket(uint32_t secretKey) : secretKey(secretKey) {}

    static std::shared_ptr<Packet> create(uint32_t secretKey) {
        return std::make_shared<ClaimThreadPacket>(secretKey);
    }

    uint32_t secretKey;
};

GLOBED_SERIALIZABLE_STRUCT(ClaimThreadPacket, (secretKey));

class KeepaliveTCPPacket : public Packet {
    GLOBED_PACKET(10006, false, true)

    KeepaliveTCPPacket() {}

    static std::shared_ptr<Packet> create() {
        return std::make_shared<KeepaliveTCPPacket>();
    }
};

GLOBED_SERIALIZABLE_STRUCT(KeepaliveTCPPacket, ());

class ConnectionTestPacket : public Packet {
    GLOBED_PACKET(10010, false, false)

    ConnectionTestPacket() {}
    ConnectionTestPacket(uint32_t uid, util::data::bytevector&& vec) : uid(uid), data(std::move(vec)) {}

    static std::shared_ptr<Packet> create(uint32_t uid, util::data::bytevector&& vec) {
        return std::make_shared<ConnectionTestPacket>(uid, std::move(vec));
    }

    uint32_t uid;
    util::data::bytevector data;
};

GLOBED_SERIALIZABLE_STRUCT(ConnectionTestPacket, (uid, data));
