#pragma once
#include <string>
#include <stdint.h>

namespace globed {

struct UserRole {
    uint8_t id;
    std::string stringId;
    std::string icon;
    std::string nameColor; // TODO: RichColor
};

}