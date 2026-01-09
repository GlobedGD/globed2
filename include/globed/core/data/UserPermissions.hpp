#pragma once

namespace globed {

struct UserPermissions {
    bool isModerator;
    bool canMute;
    bool canBan;
    bool canSetPassword;
    bool canEditRoles;
    bool canSendFeatures;
    bool canRateFeatures;
    bool canNameRooms;
};

}