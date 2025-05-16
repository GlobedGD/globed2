#pragma once
#include <data/packets/packet.hpp>
#include <data/types/admin.hpp>
#include <data/types/gd.hpp>
#include <data/types/user.hpp>

// 19000 - AdminAuthPacket
class AdminAuthPacket : public Packet {
    GLOBED_PACKET(19000, AdminAuthPacket, true, true)

    AdminAuthPacket() {}
    AdminAuthPacket(std::string_view key) : key(key) {}

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
    AdminSendNoticePacket(AdminSendNoticeType ptype, uint32_t roomId, LevelId levelId, std::string_view player, std::string_view message, bool canReply, bool justEstimate)
        : ptype(ptype), roomId(roomId), levelId(levelId), player(player), message(message), canReply(canReply), justEstimate(justEstimate) {}

    AdminSendNoticeType ptype;
    uint32_t roomId;
    LevelId levelId;
    std::string player;
    std::string message;
    bool canReply;
    bool justEstimate;
};

GLOBED_SERIALIZABLE_STRUCT(AdminSendNoticePacket, (
    ptype, roomId, levelId, player, message, canReply, justEstimate
));

// 19002 - AdminDisconnectPacket
class AdminDisconnectPacket : public Packet {
    GLOBED_PACKET(19002, AdminDisconnectPacket, false, true)

    AdminDisconnectPacket() {}
    AdminDisconnectPacket(std::string_view player, std::string_view message)
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
    AdminGetUserStatePacket(std::string_view player) : player(player) {}

    std::string player;
};

GLOBED_SERIALIZABLE_STRUCT(AdminGetUserStatePacket, (
    player
));

// 19005 - AdminSendFeaturedLevelPacket
class AdminSendFeaturedLevelPacket : public Packet {
    GLOBED_PACKET(19005, AdminSendFeaturedLevelPacket, false, false)

    AdminSendFeaturedLevelPacket() {}
    AdminSendFeaturedLevelPacket(std::string_view levelName, uint32_t levelID, std::string_view levelAuthor, int32_t difficulty, int32_t rateTier, const std::optional<std::string>& notes)
        : levelName(levelName), levelID(levelID), levelAuthor(levelAuthor), difficulty(difficulty), rateTier(rateTier), notes(notes) {}

    std::string levelName;
    uint32_t levelID;
    std::string levelAuthor;
    int32_t difficulty;
    int32_t rateTier;
    std::optional<std::string> notes;
};

GLOBED_SERIALIZABLE_STRUCT(AdminSendFeaturedLevelPacket, (
    levelName, levelID, levelAuthor, difficulty, rateTier, notes
));

// 19010 - AdminUpdateUsernamePacket
class AdminUpdateUsernamePacket : public Packet {
    GLOBED_PACKET(19010, AdminUpdateUsernamePacket, true, false)

    AdminUpdateUsernamePacket() {}
    AdminUpdateUsernamePacket(int32_t accountId, std::string_view name) : accountId(accountId), name(name) {}

    int32_t accountId;
    std::string name;
};

GLOBED_SERIALIZABLE_STRUCT(AdminUpdateUsernamePacket, (
    accountId, name
));

// 19011 - AdminSetNameColorPacket
class AdminSetNameColorPacket : public Packet {
    GLOBED_PACKET(19011, AdminSetNameColorPacket, true, false)

    AdminSetNameColorPacket() {}
    AdminSetNameColorPacket(int32_t accountId, cocos2d::ccColor3B color) : accountId(accountId), color(color) {}

    int32_t accountId;
    cocos2d::ccColor3B color;
};

GLOBED_SERIALIZABLE_STRUCT(AdminSetNameColorPacket, (
    accountId, color
));

// 19012 - AdminSetUserRolesPacket
class AdminSetUserRolesPacket : public Packet {
    GLOBED_PACKET(19012, AdminSetUserRolesPacket, true, false)

    AdminSetUserRolesPacket() {}
    AdminSetUserRolesPacket(int32_t accountId, const std::vector<std::string>& roles) : accountId(accountId), roles(roles) {}

    int32_t accountId;
    std::vector<std::string> roles;
};

GLOBED_SERIALIZABLE_STRUCT(AdminSetUserRolesPacket, (
    accountId, roles
));

// 19013 - AdminPunishUserPacket
class AdminPunishUserPacket : public Packet {
    GLOBED_PACKET(19013, AdminPunishUserPacket, true, false)

    AdminPunishUserPacket() {}
    AdminPunishUserPacket(int32_t accountId, bool isBan, std::string_view reason, uint64_t expiresAt) : accountId(accountId), isBan(isBan), reason(reason), expiresAt(expiresAt) {}

    int32_t accountId;
    bool isBan;
    std::string reason;
    uint64_t expiresAt;
};

GLOBED_SERIALIZABLE_STRUCT(AdminPunishUserPacket, (
    accountId, isBan, reason, expiresAt
));

// 19014 - AdminRemovePunishmentPacket
class AdminRemovePunishmentPacket : public Packet {
    GLOBED_PACKET(19014, AdminRemovePunishmentPacket, true, false)

    AdminRemovePunishmentPacket() {}
    AdminRemovePunishmentPacket(int32_t accountId, bool isBan) : accountId(accountId), isBan(isBan) {}

    int32_t accountId;
    bool isBan;
};

GLOBED_SERIALIZABLE_STRUCT(AdminRemovePunishmentPacket, (
    accountId, isBan,
));

// 19015 - AdminWhitelistPacket
class AdminWhitelistPacket : public Packet {
    GLOBED_PACKET(19015, AdminWhitelistPacket, true, false)

    AdminWhitelistPacket() {}
    AdminWhitelistPacket(int32_t accountId, bool state) : accountId(accountId), state(state) {}

    int32_t accountId;
    bool state;
};

GLOBED_SERIALIZABLE_STRUCT(AdminWhitelistPacket, (
    accountId, state,
));

// 19016 - AdminSetAdminPasswordPacket
class AdminSetAdminPasswordPacket : public Packet {
    GLOBED_PACKET(19016, AdminSetAdminPasswordPacket, true, false)

    AdminSetAdminPasswordPacket() {}
    AdminSetAdminPasswordPacket(int32_t accountId, std::string_view newPassword) : accountId(accountId), newPassword(newPassword) {}

    int32_t accountId;
    std::string newPassword;
};

GLOBED_SERIALIZABLE_STRUCT(AdminSetAdminPasswordPacket, (
    accountId, newPassword
));

// 19017 - AdminEditPunishmentPacket
class AdminEditPunishmentPacket : public Packet {
    GLOBED_PACKET(19017, AdminEditPunishmentPacket, true, false)

    AdminEditPunishmentPacket() {}
    AdminEditPunishmentPacket(int32_t accountId, bool isBan, std::string_view reason, uint64_t expiresAt) : accountId(accountId), isBan(isBan), reason(reason), expiresAt(expiresAt) {}

    int32_t accountId;
    bool isBan;
    std::string reason;
    uint64_t expiresAt;
};

GLOBED_SERIALIZABLE_STRUCT(AdminEditPunishmentPacket, (
    accountId, isBan, reason, expiresAt
));

// 19018 - AdminGetPunishmentHistoryPacket
class AdminGetPunishmentHistoryPacket : public Packet {
    GLOBED_PACKET(19018, AdminGetPunishmentHistoryPacket, true, false)

    AdminGetPunishmentHistoryPacket() {}
    AdminGetPunishmentHistoryPacket(int32_t accountId) : accountId(accountId) {}

    int32_t accountId;
};

GLOBED_SERIALIZABLE_STRUCT(AdminGetPunishmentHistoryPacket, (
    accountId
));