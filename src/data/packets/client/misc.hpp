#pragma once
#include <data/packets/packet.hpp>
#include <data/types/gd.hpp>

/*
* RawPacket is a special packet. It is not an actual specific packet and has no consistent representation.
* Example usage and explanation can be found in `ui/hooks/play_layer.hpp` in the audio callback.
*/
class RawPacket : public Packet {
public:
    RawPacket(packetid_t id, bool encrypted, bool tcp, ByteBuffer&& buffer) : id(id), encrypted(encrypted), tcp(tcp), buffer(std::move(buffer)) {}

    packetid_t getPacketId() const override {
        return id;
    }

    bool getUseTcp() const override {
        return tcp;
    }

    bool getEncrypted() const override {
        return encrypted;
    }

    GLOBED_PACKET_ENCODE {
        buf.writeBytes(buffer.getDataRef());
    }

    static std::shared_ptr<Packet> create(packetid_t id, bool encrypted, bool tcp, ByteBuffer&& buffer) {
        return std::make_shared<RawPacket>(id, encrypted, tcp, std::move(buffer));
    }

    template <typename T>
    static std::shared_ptr<Packet> create(ByteBuffer&& buffer) {
        return std::make_shared<RawPacket>(T::PACKET_ID, T::ENCRYPTED, T::SHOULD_USE_TCP, std::move(buffer));
    }

    packetid_t id;
    bool encrypted;
    bool tcp;
    mutable ByteBuffer buffer;
};