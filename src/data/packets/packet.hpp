#pragma once
#include <data/bytebuffer.hpp>

using packetid_t = uint16_t;

#define GLOBED_PACKET(id, enc, tcp) \
    public: \
    static constexpr packetid_t PACKET_ID = id; \
    static constexpr bool SHOULD_USE_TCP = tcp; \
    static constexpr bool ENCRYPTED = enc; \
    packetid_t getPacketId() const override { return this->PACKET_ID; } \
    bool getUseTcp() const override { return this->SHOULD_USE_TCP; } \
    bool getEncrypted() const override { return this->ENCRYPTED; }

#define GLOBED_PACKET_ENCODE void encode(ByteBuffer& buf) const override
#define GLOBED_PACKET_DECODE void decode(ByteBuffer& buf) override

class Packet {
public:
    virtual ~Packet() {}
    // Encodes the packet into a bytebuffer
    virtual void encode(ByteBuffer& buf) const {
        GLOBED_UNIMPL(std::string("Encoding unimplemented for packet ") + std::to_string(this->getPacketId()))
    };

    // Decodes the packet from a bytebuffer
    virtual void decode(ByteBuffer& buf) {
        GLOBED_UNIMPL(std::string("Decoding unimplemented for packet ") + std::to_string(this->getPacketId()))
    };

    virtual packetid_t getPacketId() const = 0;
    virtual bool getUseTcp() const = 0;
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