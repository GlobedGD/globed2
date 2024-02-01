#pragma once
#include <data/packets/packet.hpp>

class AdminAuthPacket : public Packet {
    GLOBED_PACKET(19000, true)

    GLOBED_PACKET_ENCODE { buf.writeString(key); }

    AdminAuthPacket(const std::string_view key) : key(key) {}

    static std::shared_ptr<Packet> create(const std::string_view key) {
        return std::make_shared<AdminAuthPacket>(key);
    }

    std::string key;
};

enum class AdminSendNoticeType : uint8_t {
    Everyone = 0,
    RoomOrLevel = 1,
    Person = 2,
};

class AdminSendNoticePacket : public Packet {
    GLOBED_PACKET(19001, true)

    GLOBED_PACKET_ENCODE {
        buf.writeEnum(ptype);
        buf.writeU32(roomId);
        buf.writeI32(levelId);
        buf.writeString(player);
        buf.writeString(message);
    }

    AdminSendNoticePacket(AdminSendNoticeType ptype, uint32_t roomId, int levelId, const std::string_view player, const std::string_view message)
        : ptype(ptype), roomId(roomId), levelId(levelId), player(player), message(message) {}

    static std::shared_ptr<Packet> create(AdminSendNoticeType ptype, uint32_t roomId, int levelId, const std::string_view player, const std::string_view message) {
        return std::make_shared<AdminSendNoticePacket>(ptype, roomId, levelId, player, message);
    }

    AdminSendNoticeType ptype;
    uint32_t roomId;
    int levelId;
    std::string player;
    std::string message;
};
