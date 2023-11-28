#pragma once
#include <data/bytebuffer.hpp>
#include <defs.hpp>

using packetid_t = uint16_t;
#define GLOBED_PACKET(id,enc) \
    public: \
    static constexpr packetid_t PACKET_ID = id; \
    static constexpr bool ENCRYPTED = enc; \
    packetid_t getPacketId() const override { return this->PACKET_ID; } \
    bool getEncrypted() const override { return this->ENCRYPTED; }

#define GLOBED_PACKET_ENCODE_UNIMPL \
    GLOBED_PACKET_ENCODE { \
        GLOBED_UNIMPL(std::string("Encoding unimplemented for packet ") + std::to_string(getPacketId())) \
    }

#define GLOBED_PACKET_DECODE_UNIMPL \
    GLOBED_PACKET_DECODE { \
        GLOBED_UNIMPL(std::string("Decoding unimplemented for packet ") + std::to_string(getPacketId())) \
    }

#define GLOBED_PACKET_ENCODE void encode(ByteBuffer& buf) const override
#define GLOBED_PACKET_DECODE void decode(ByteBuffer& buf) override

class Packet {
public:
    virtual ~Packet() {}
    // Encodes the packet into a bytebuffer
    virtual void encode(ByteBuffer& buf) const = 0;
    // Decodes the packet from a bytebuffer
    virtual void decode(ByteBuffer& buf) = 0;

    virtual packetid_t getPacketId() const = 0;
    virtual bool getEncrypted() const = 0;
};

class PacketHeader {
public:
    static constexpr size_t SIZE = sizeof(packetid_t) + sizeof(bool);

    GLOBED_ENCODE {
        buf.writeU16(id);
        buf.writeBool(encrypted);
    }

    GLOBED_DECODE {
        id = buf.readU16();
        encrypted = buf.readBool();
    }
    
    packetid_t id;
    bool encrypted;
};