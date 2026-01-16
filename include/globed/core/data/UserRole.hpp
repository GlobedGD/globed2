#pragma once
#include "MultiColor.hpp"

#include <stdint.h>
#include <string>

namespace globed {

struct UserRole {
    uint8_t id;
    std::string stringId;
    std::string icon;
    MultiColor nameColor;
    bool hide;
};

} // namespace globed