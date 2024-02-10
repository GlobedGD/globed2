#pragma once
#include <data/packets/packet.hpp>
#include <data/types/crypto.hpp>
#include <data/types/gd.hpp>

class PingPacket : public Packet {
    GLOBED_PACKET(10000, false)

    GLOBED_PACKET_ENCODE { buf.writeU32(id); }

    PingPacket(uint32_t _id) : id(_id) {}

    static std::shared_ptr<Packet> create(uint32_t id) {
        return std::make_shared<PingPacket>(id);
    }

    uint32_t id;
};

class CryptoHandshakeStartPacket : public Packet {
    GLOBED_PACKET(10001, false)

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
    GLOBED_PACKET(10002, false)

    GLOBED_PACKET_ENCODE {}

    KeepalivePacket() {}
    static std::shared_ptr<Packet> create() {
        return std::make_shared<KeepalivePacket>();
    }
};

class LoginPacket : public Packet {
    GLOBED_PACKET(10003, true)

    GLOBED_PACKET_ENCODE {
        buf.writeI32(accountId);
        buf.writeString(name);
        buf.writeString(token);
        buf.writeValue(icons);
    }

    LoginPacket(int32_t accid, const std::string_view name, const std::string_view token, const PlayerIconData& icons)
        : accountId(accid), name(name), token(token), icons(icons) {}

    static std::shared_ptr<Packet> create(int32_t accid, const std::string_view name, const std::string_view token, const PlayerIconData& icons) {
        return std::make_shared<LoginPacket>(accid, name, token, icons);
    }

    int32_t accountId;
    std::string name;
    std::string token;
    PlayerIconData icons;
};

class DisconnectPacket : public Packet {
    GLOBED_PACKET(10004, false)

    GLOBED_PACKET_ENCODE {}

    DisconnectPacket() {}
    static std::shared_ptr<Packet> create() {
        return std::make_shared<DisconnectPacket>();
    }
};

class ConnectionTestPacket : public Packet {
    GLOBED_PACKET(10010, false)

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
