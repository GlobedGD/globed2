#pragma once
#include "MultiColor.hpp"

#include <string>
#include <stdint.h>

namespace globed {

struct UserRole {
    uint8_t id;
    std::string stringId;
    std::string icon;
    MultiColor nameColor;
    bool hide;
};

}