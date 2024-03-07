#pragma once
#include <data/packets/packet.hpp>
#include <data/types/admin.hpp>
#include <data/types/gd.hpp>

class AdminAuthPacket : public Packet {
    GLOBED_PACKET(19000, true, true)

    AdminAuthPacket() {}
    AdminAuthPacket(const std::string_view key) : key(key) {}

    static std::shared_ptr<Packet> create(const std::string_view key) {
        return std::make_shared<AdminAuthPacket>(key);
    }

    std::string key;
};

GLOBED_SERIALIZABLE_STRUCT(AdminAuthPacket, (key));

enum class AdminSendNoticeType : uint8_t {
    Everyone = 0,
    RoomOrLevel = 1,
    Person = 2,
};

GLOBED_SERIALIZABLE_ENUM(AdminSendNoticeType, Everyone, RoomOrLevel, Person);

class AdminSendNoticePacket : public Packet {
    GLOBED_PACKET(19001, true, true)

    AdminSendNoticePacket() {}
    AdminSendNoticePacket(AdminSendNoticeType ptype, uint32_t roomId, LevelId levelId, const std::string_view player, const std::string_view message)
        : ptype(ptype), roomId(roomId), levelId(levelId), player(player), message(message) {}

    static std::shared_ptr<Packet> create(AdminSendNoticeType ptype, uint32_t roomId, LevelId levelId, const std::string_view player, const std::string_view message) {
        return std::make_shared<AdminSendNoticePacket>(ptype, roomId, levelId, player, message);
    }

    AdminSendNoticeType ptype;
    uint32_t roomId;
    LevelId levelId;
    std::string player;
    std::string message;
};

GLOBED_SERIALIZABLE_STRUCT(AdminSendNoticePacket, (
    ptype, roomId, levelId, player, message
));

class AdminDisconnectPacket : public Packet {
    GLOBED_PACKET(19002, false, true)

    AdminDisconnectPacket() {}
    AdminDisconnectPacket(const std::string_view player, const std::string_view message)
        : player(player), message(message) {}

    static std::shared_ptr<Packet> create(const std::string_view player, const std::string_view message) {
        return std::make_shared<AdminDisconnectPacket>(player, message);
    }

    std::string player, message;
};

GLOBED_SERIALIZABLE_STRUCT(AdminDisconnectPacket, (
    player, message
));

class AdminGetUserStatePacket : public Packet {
    GLOBED_PACKET(19003, false, true)

    AdminGetUserStatePacket() {}
    AdminGetUserStatePacket(const std::string_view player) : player(player) {}

    static std::shared_ptr<Packet> create(const std::string_view player) {
        return std::make_shared<AdminGetUserStatePacket>(player);
    }

    std::string player;
};

GLOBED_SERIALIZABLE_STRUCT(AdminGetUserStatePacket, (
    player
));

class AdminUpdateUserPacket : public Packet {
    GLOBED_PACKET(19004, true, true)

    AdminUpdateUserPacket() {}
    AdminUpdateUserPacket(const UserEntry& entry) : userEntry(entry) {}

    static std::shared_ptr<Packet> create(const UserEntry& entry) {
        return std::make_shared<AdminUpdateUserPacket>(entry);
    }

    UserEntry userEntry;
};

GLOBED_SERIALIZABLE_STRUCT(AdminUpdateUserPacket, (
    userEntry
));
