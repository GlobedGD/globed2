#pragma once
#include <data/packets/packet.hpp>
#include <data/types/admin.hpp>
#include <data/types/gd.hpp>

// 19000 - AdminAuthPacket
class AdminAuthPacket : public Packet {
    GLOBED_PACKET(19000, AdminAuthPacket, true, true)

    AdminAuthPacket() {}
    AdminAuthPacket(const std::string_view key) : key(key) {}

    std::string key;
};

GLOBED_SERIALIZABLE_STRUCT(AdminAuthPacket, (key));

enum class AdminSendNoticeType : uint8_t {
    Everyone = 0,
    RoomOrLevel = 1,
    Person = 2,
};

GLOBED_SERIALIZABLE_ENUM(AdminSendNoticeType, Everyone, RoomOrLevel, Person);

// 19001 - AdminSendNoticePacket
class AdminSendNoticePacket : public Packet {
    GLOBED_PACKET(19001, AdminSendNoticePacket, true, true)

    AdminSendNoticePacket() {}
    AdminSendNoticePacket(AdminSendNoticeType ptype, uint32_t roomId, LevelId levelId, const std::string_view player, const std::string_view message)
        : ptype(ptype), roomId(roomId), levelId(levelId), player(player), message(message) {}

    AdminSendNoticeType ptype;
    uint32_t roomId;
    LevelId levelId;
    std::string player;
    std::string message;
};

GLOBED_SERIALIZABLE_STRUCT(AdminSendNoticePacket, (
    ptype, roomId, levelId, player, message
));

// 19002 - AdminDisconnectPacket
class AdminDisconnectPacket : public Packet {
    GLOBED_PACKET(19002, AdminDisconnectPacket, false, true)

    AdminDisconnectPacket() {}
    AdminDisconnectPacket(const std::string_view player, const std::string_view message)
        : player(player), message(message) {}

    std::string player, message;
};

GLOBED_SERIALIZABLE_STRUCT(AdminDisconnectPacket, (
    player, message
));

// 19003 - AdminGetUserStatePacket
class AdminGetUserStatePacket : public Packet {
    GLOBED_PACKET(19003, AdminGetUserStatePacket, false, true)

    AdminGetUserStatePacket() {}
    AdminGetUserStatePacket(const std::string_view player) : player(player) {}

    std::string player;
};

GLOBED_SERIALIZABLE_STRUCT(AdminGetUserStatePacket, (
    player
));

// 19004 - AdminUpdateUserPacket
class AdminUpdateUserPacket : public Packet {
    GLOBED_PACKET(19004, AdminUpdateUserPacket, true, true)

    AdminUpdateUserPacket() {}
    AdminUpdateUserPacket(const UserEntry& entry) : userEntry(entry) {}

    UserEntry userEntry;
};

GLOBED_SERIALIZABLE_STRUCT(AdminUpdateUserPacket, (
    userEntry
));
