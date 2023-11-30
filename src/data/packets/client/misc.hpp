#pragma once
#include <defs.hpp>
#include <data/packets/packet.hpp>
#include <data/types/gd.hpp>

/*
* RawPacket is a special packet. It is not an actual specific packet and has no consistent representation.
* Example usage and explanation can be found in `ui/hooks/play_layer.hpp` in the audio callback function.
*/
class RawPacket : public Packet {
public:
    RawPacket(packetid_t id, bool encrypted, ByteBuffer&& buffer) : id(id), encrypted(encrypted), buffer(std::move(buffer)) {}

    packetid_t getPacketId() const override {
        return id;
    }

    bool getEncrypted() const override {
        return encrypted;
    }

    GLOBED_PACKET_ENCODE {
        buf.writeBytes(buffer.getDataRef());
    }

    GLOBED_PACKET_DECODE_UNIMPL

    static std::shared_ptr<RawPacket> create(packetid_t id, bool encrypted, ByteBuffer&& buffer) {
        return std::make_shared<RawPacket>(id, encrypted, std::move(buffer));
    }

    packetid_t id;
    bool encrypted;
    mutable ByteBuffer buffer;
};