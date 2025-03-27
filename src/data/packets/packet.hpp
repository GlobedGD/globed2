#pragma once

#include <defs/minimal_geode.hpp>
#include <defs/assert.hpp>
#include <data/bytebuffer.hpp>

using packetid_t = uint16_t;

#define GLOBED_PACKET(id, name, enc, tcp) \
    public: \
    static constexpr packetid_t PACKET_ID = id; \
    static constexpr bool SHOULD_USE_TCP = tcp; \
    static constexpr bool ENCRYPTED = enc; \
    static constexpr const char* PACKET_NAME = #name; \
    packetid_t getPacketId() const override { return this->PACKET_ID; } \
    bool getUseTcp() const override { return this->SHOULD_USE_TCP; } \
    bool getEncrypted() const override { return this->ENCRYPTED; } \
    const char* getPacketName() const override { return this->PACKET_NAME; } \
    void encode(ByteBuffer& buf) const override { \
        using InstTy = typename std::remove_reference_t<decltype(*this)>; \
        using NonCvTy = typename std::remove_cv_t<InstTy>; \
        buf.writeValue<NonCvTy>(*this); \
    } \
    ByteBuffer::DecodeResult<> decode(ByteBuffer& buf) override { \
        GLOBED_UNWRAP_INTO(buf.readValue<std::remove_reference_t<decltype(*this)>>(), *this); \
        return Ok(); \
    } \
    template <typename... Args> \
    static std::shared_ptr<Packet> create(Args&&... args) { \
        return std::make_shared<name>(std::forward<Args>(args)...); \
    }
class Packet {
public:
    virtual ~Packet() {}
    // Encodes the packet into a bytebuffer
    virtual void encode(ByteBuffer& buf) const = 0;

    // Decodes the packet from a bytebuffer
    virtual ByteBuffer::DecodeResult<> decode(ByteBuffer& buf) = 0;

    virtual packetid_t getPacketId() const = 0;
    virtual bool getUseTcp() const = 0;
    virtual bool getEncrypted() const = 0;
    virtual const char* getPacketName() const = 0;

    template <typename T>
    requires std::is_base_of_v<Packet, T>
    bool isInstanceOf() {
        return this->getPacketId() == T::PACKET_ID;
    }

    // Downcast a `Packet*` to a specific instance. Returns `nullptr` if this packet is not an instance of `T`
    template <typename T>
    requires std::is_base_of_v<Packet, T>
    T* tryDowncast() {
        if constexpr (std::is_same_v<T, Packet>) {
            return this;
        } else {
            if (!this->isInstanceOf<T>()) {
                return nullptr;
            }

            return static_cast<T*>(this);
        }
    }
};

struct PacketHeader {
    static constexpr size_t SIZE = sizeof(packetid_t) + sizeof(bool);

    packetid_t id;
    bool encrypted;
};

GLOBED_SERIALIZABLE_STRUCT(PacketHeader, (id, encrypted));
