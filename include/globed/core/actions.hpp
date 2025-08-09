#pragma once
#include <globed/core/SessionId.hpp>
#include <globed/core/data/RoomPlayer.hpp>

namespace globed {

void warpToSession(SessionId session, bool openLevel = false);
void warpToLevel(int level, bool openLevel = false);

void openUserProfile(int accountId, int userId = 0, std::string_view username = "");
void openUserProfile(const RoomPlayer& player);
void openUserProfile(const PlayerAccountData& player);
void openModPanel(int accountId = 0);

SessionId _getWarpContext();
void _clearWarpContext();

}
