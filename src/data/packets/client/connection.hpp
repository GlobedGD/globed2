#pragma once
#include <data/packets/packet.hpp>
#include <data/types/crypto.hpp>
#include <data/types/gd.hpp>

class PingPacket : public Packet {
    GLOBED_PACKET(10000, false, false)

    GLOBED_PACKET_ENCODE { buf.writeU32(id); }

    PingPacket(uint32_t _id) : id(_id) {}

    static std::shared_ptr<Packet> create(uint32_t id) {
        return std::make_shared<PingPacket>(id);
    }

    uint32_t id;
};

class CryptoHandshakeStartPacket : public Packet {
    GLOBED_PACKET(10001, false, true)

    GLOBED_PACKET_ENCODE {
        buf.writeU16(protocol);
        buf.writeValue(key);
    }

    CryptoHandshakeStartPacket(uint16_t _protocol, CryptoPublicKey _key) : protocol(_protocol), key(_key) {}

    static std::shared_ptr<Packet> create(uint16_t protocol, CryptoPublicKey key) {
        return std::make_shared<CryptoHandshakeStartPacket>(protocol, key);
    }

    uint16_t protocol;
    CryptoPublicKey key;
};

class KeepalivePacket : public Packet {
    GLOBED_PACKET(10002, false, false)

    GLOBED_PACKET_ENCODE {}

    KeepalivePacket() {}
    static std::shared_ptr<Packet> create() {
        return std::make_shared<KeepalivePacket>();
    }
};

class LoginPacket : public Packet {
    GLOBED_PACKET(10003, true, true)

    GLOBED_PACKET_ENCODE {
        buf.writeU32(secretKey);
        buf.writeI32(accountId);
        buf.writeI32(userId);
        buf.writeString(name);
        buf.writeString(token);
        buf.writeValue(icons);
        buf.writeU16(fragmentationLimit);
    }

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

class DisconnectPacket : public Packet {
    GLOBED_PACKET(10004, false, false)

    GLOBED_PACKET_ENCODE {}

    DisconnectPacket() {}
    static std::shared_ptr<Packet> create() {
        return std::make_shared<DisconnectPacket>();
    }
};

class ClaimThreadPacket : public Packet {
    GLOBED_PACKET(10005, false, false)

    GLOBED_PACKET_ENCODE {
        buf.writeU32(secretKey);
    }

    ClaimThreadPacket(uint32_t secretKey) : secretKey(secretKey) {}

    static std::shared_ptr<Packet> create(uint32_t secretKey) {
        return std::make_shared<ClaimThreadPacket>(secretKey);
    }

    uint32_t secretKey;
};

class KeepaliveTCPPacket : public Packet {
    GLOBED_PACKET(10006, false, true)

    GLOBED_PACKET_ENCODE {}

    KeepaliveTCPPacket() {}
    static std::shared_ptr<Packet> create() {
        return std::make_shared<KeepaliveTCPPacket>();
    }
};

class ConnectionTestPacket : public Packet {
    GLOBED_PACKET(10010, false, false)

    GLOBED_PACKET_ENCODE {
        buf.writeU32(uid);
        buf.writeByteArray(data);
    }

    ConnectionTestPacket(uint32_t uid, util::data::bytevector&& vec) : uid(uid), data(std::move(vec)) {}
    static std::shared_ptr<Packet> create(uint32_t uid, util::data::bytevector&& vec) {
        return std::make_shared<ConnectionTestPacket>(uid, std::move(vec));
    }

    uint32_t uid;
    util::data::bytevector data;
};
