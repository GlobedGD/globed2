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
    bool admin;
};

GLOBED_SERIALIZABLE_STRUCT(ServerRole, (
    id, priority, badgeIcon, nameColor, chatColor, notices, noticesToEveryone, kick, kickEveryone, mute, ban, editRole, admin
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
    bool admin;
};

GLOBED_SERIALIZABLE_STRUCT(ComputedRole, (
    priority, badgeIcon, nameColor, chatColor, notices, noticesToEveryone, kick, kickEveryone, mute, ban, editRole, admin
));
