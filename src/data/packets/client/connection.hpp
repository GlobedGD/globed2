#pragma once
#include <data/packets/packet.hpp>
#include <data/types/crypto.hpp>

class PingPacket : public Packet {
    GLOBED_PACKET(10000, false)

    GLOBED_PACKET_ENCODE { buf.writeU32(id); }
    GLOBED_PACKET_DECODE_UNIMPL

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
    GLOBED_PACKET_DECODE_UNIMPL

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
    GLOBED_PACKET_DECODE_UNIMPL

    KeepalivePacket() {}
    static std::shared_ptr<Packet> create() {
        return std::make_shared<KeepalivePacket>();
    }
};

class LoginPacket : public Packet {
    GLOBED_PACKET(10003, true)

    GLOBED_PACKET_ENCODE {
        buf.writeI32(accountId);
        buf.writeString(token);
    }

    GLOBED_PACKET_DECODE_UNIMPL
    
    LoginPacket(int32_t _accid, const std::string& _token) : accountId(_accid), token(_token) {}

    static std::shared_ptr<Packet> create(int32_t accid, const std::string& token) {
        return std::make_shared<LoginPacket>(accid, token);
    }

    int32_t accountId;
    std::string token;
};

class DisconnectPacket : public Packet {
    GLOBED_PACKET(10004, false)

    GLOBED_PACKET_ENCODE {}
    GLOBED_PACKET_DECODE_UNIMPL

    DisconnectPacket() {}
    static std::shared_ptr<Packet> create() {
        return std::make_shared<DisconnectPacket>();
    }
};