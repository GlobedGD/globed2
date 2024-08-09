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
    int32_t priority;
    std::string badgeIcon;
    std::optional<RichColor> nameColor;
    std::optional<cocos2d::ccColor3B> chatColor;

    bool notices, noticesToEveryone;
    bool kick, kickEveryone;
    bool mute;
    bool ban;
    bool editRole;
    bool editFeaturedLevels;
    bool admin;

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
};

GLOBED_SERIALIZABLE_STRUCT(UserPrivacyFlags, (
    hideFromLists, noInvites, hideInGame, hideRoles
));
