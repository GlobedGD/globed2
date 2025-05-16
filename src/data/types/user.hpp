#pragma once

#include <data/bytebuffer.hpp>
#include "misc.hpp"

struct ServerRole {
    std::string id;
    int32_t priority;
    std::string badgeIcon;
    std::string nameColor;
    std::string chatColor;

    bool notices, noticesToEveryone;
    bool kick, kickEveryone;
    bool mute;
    bool ban;
    bool editRole;
    bool editFeaturedLevels;
    bool admin;
};

GLOBED_SERIALIZABLE_STRUCT(ServerRole, (
    id, priority, badgeIcon, nameColor, chatColor, notices, noticesToEveryone, kick, kickEveryone, mute, ban, editRole, editFeaturedLevels, admin
));

struct GameServerRole {
    uint8_t intId;
    ServerRole role;
};

GLOBED_SERIALIZABLE_STRUCT(GameServerRole, (
    intId, role
));

struct ComputedRole {
    int32_t priority = false;
    std::string badgeIcon;
    std::optional<RichColor> nameColor;
    std::optional<cocos2d::ccColor3B> chatColor;

    bool notices = false, noticesToEveryone = false;
    bool kick = false, kickEveryone = false;
    bool mute = false;
    bool ban = false;
    bool editRole = false;
    bool editFeaturedLevels = false;
    bool admin = false;

    bool canModerate() {
        return notices || noticesToEveryone || kick || kickEveryone || mute || ban || editRole || editFeaturedLevels || admin;
    }
};

GLOBED_SERIALIZABLE_STRUCT(ComputedRole, (
    priority, badgeIcon, nameColor, chatColor, notices, noticesToEveryone, kick, kickEveryone, mute, ban, editRole, editFeaturedLevels, admin
));

struct UserPrivacyFlags : BitfieldBase {
    bool hideFromLists;
    bool noInvites;
    bool hideInGame;
    bool hideRoles;
    bool tcpAudio;
};

GLOBED_SERIALIZABLE_BITFIELD(UserPrivacyFlags, (
    hideFromLists, noInvites, hideInGame, hideRoles, tcpAudio
));

enum class PunishmentType : uint8_t {
    Ban = 0, Mute = 1
};

GLOBED_SERIALIZABLE_ENUM(PunishmentType, Ban, Mute);

struct UserPunishment {
    int64_t id;
    int32_t accountId;
    PunishmentType type;
    std::string reason;
    int64_t expiresAt;
    std::optional<int64_t> issuedAt;
    std::optional<int32_t> issuedBy;
};

GLOBED_SERIALIZABLE_STRUCT(UserPunishment, (
    id, accountId, type, reason, expiresAt, issuedAt, issuedBy
))