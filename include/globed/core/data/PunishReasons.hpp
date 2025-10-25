#pragma once

#include <string>
#include <vector>

namespace globed {

struct PunishReasons {
    std::vector<std::string> banReasons;
    std::vector<std::string> muteReasons;
    std::vector<std::string> roomBanReasons;
};

}
