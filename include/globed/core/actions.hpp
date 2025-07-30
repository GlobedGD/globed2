#pragma once
#include <globed/core/SessionId.hpp>
#include <globed/core/data/RoomPlayer.hpp>

namespace globed {

void warpToSession(SessionId session);
void warpToLevel(int level);

void openUserProfile(int accountId, int userId = 0, std::string_view username = "");
void openUserProfile(const RoomPlayer& player);
void openUserProfile(const PlayerAccountData& player);

}
