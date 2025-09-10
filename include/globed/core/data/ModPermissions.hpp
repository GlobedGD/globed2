#pragma once

namespace globed {

struct ModPermissions {
    bool isModerator;
    bool canMute;
    bool canBan;
    bool canSetPassword;
    bool canEditRoles;
    bool canSendFeatures;
    bool canRateFeatures;
};

}