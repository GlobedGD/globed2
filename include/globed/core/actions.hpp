#pragma once
#include <globed/core/SessionId.hpp>

namespace globed {

void warpToSession(SessionId session);
void warpToLevel(int level);

void openUserProfile(int accountId, int userId = 0, std::string_view username = "");

}
