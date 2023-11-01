#pragma once
#include <data/bytebuffer.hpp>
#include <defs.hpp>

using packetid_t = uint16_t;
#define GLOBED_PACKET(id,enc) \
    public: \
    packetid_t getPacketId() const override { return id; } \
    bool getEncrypted() const override { return enc; }

#define GLOBED_ENCODE_UNIMPL \
    GLOBED_ENCODE { \
        GLOBED_UNIMPL(std::string("Encoding unimplemented for packet ") + std::to_string(getPacketId())) \
    }

#define GLOBED_DECODE_UNIMPL \
    GLOBED_DECODE { \
        GLOBED_UNIMPL(std::string("Decoding unimplemented for packet ") + std::to_string(getPacketId())) \
    }

#define GLOBED_ENCODE void encode(ByteBuffer& buf) const override
#define GLOBED_DECODE void decode(ByteBuffer& buf) override

class Packet {
public:
    virtual ~Packet() {}
    // Encodes the packet into a bytebuffer
    virtual void encode(ByteBuffer& buf) const = 0;
    // Decodes the packet from a bytebuffer
    virtual void decode(ByteBuffer& buf) = 0;

    virtual packetid_t getPacketId() const = 0;
    virtual bool getEncrypted() const = 0;

    static const size_t HEADER_LEN = sizeof(packetid_t) + 1;
};