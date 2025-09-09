#pragma once

#include "MultiColor.hpp"

namespace globed {

struct SpecialUserData {
    std::vector<uint8_t> roleIds;
    std::optional<MultiColor> nameColor;
};

}