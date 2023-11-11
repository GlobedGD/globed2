#pragma once
#include <data/packets/packet.hpp>
#include <data/types/crypto.hpp>

class PingPacket : public Packet {
    GLOBED_PACKET(10000, false)

    GLOBED_PACKET_ENCODE {
        buf.writeU32(id);
    }

    GLOBED_PACKET_DECODE_UNIMPL

    PingPacket(uint32_t _id) : id(_id) {}

    static PingPacket* create(uint32_t id) {
        return new PingPacket(id);
    }

    uint32_t id;
};

class CryptoHandshakeStartPacket : public Packet {
    GLOBED_PACKET(10001, false)

    GLOBED_PACKET_ENCODE {
        buf.writeI32(accountId);
        buf.writeString(token);
        buf.writeValue(key);
    }

    GLOBED_PACKET_DECODE_UNIMPL

    CryptoHandshakeStartPacket(CryptoPublicKey _key, int32_t _accid, const std::string& _token) : key(_key), accountId(_accid), token(_token) {}

    static CryptoHandshakeStartPacket* create(CryptoPublicKey key, int32_t accid, const std::string& token) {
        return new CryptoHandshakeStartPacket(key, accid, token);
    }

    int32_t accountId;
    std::string token;
    CryptoPublicKey key;
};

class KeepalivePacket : public Packet {
    GLOBED_PACKET(10002, false)
    
    GLOBED_PACKET_ENCODE {}
    GLOBED_PACKET_DECODE_UNIMPL

    KeepalivePacket() {}
    static KeepalivePacket* create() {
        return new KeepalivePacket;
    }
};