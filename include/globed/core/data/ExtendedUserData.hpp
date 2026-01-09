#pragma once

#include "MultiColor.hpp"
#include "UserPermissions.hpp"

namespace globed {

struct ExtendedUserData {
    std::string newToken;
    std::vector<uint8_t> roleIds;
    std::optional<MultiColor> nameColor;
    UserPermissions permissions;
};

}