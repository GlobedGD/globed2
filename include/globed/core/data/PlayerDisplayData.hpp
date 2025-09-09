#pragma once
#include "PlayerIconData.hpp"
#include "SpecialUserData.hpp"
#include <string>

namespace globed {

struct PlayerDisplayData {
    int accountId;
    int userId;
    std::string username;
    PlayerIconData icons;
    std::optional<SpecialUserData> specialUserData;
};

}
