#pragma once

#include "ColorId.hpp"

namespace globed {

struct CreditsUser {
    int accountId;
    int userId;
    std::string username;
    std::string displayName;
    int16_t cube;
    ColorId<uint16_t> color1;
    ColorId<uint16_t> color2;
    ColorId<uint16_t> glowColor;
};

struct CreditsCategory {
    std::string name;
    std::vector<CreditsUser> users;
};

}