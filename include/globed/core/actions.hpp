#pragma once
#include <globed/config.hpp>
#include <globed/core/SessionId.hpp>
#include <globed/core/data/RoomPlayer.hpp>

namespace globed {

GLOBED_DLL void warpToSession(SessionId session, bool openLevel = false, bool force = false);
GLOBED_DLL void warpToLevel(int level, bool openLevel = false);

GLOBED_DLL void openUserProfile(int accountId, int userId = 0, std::string_view username = "");
GLOBED_DLL void openUserProfile(const RoomPlayer &player);
GLOBED_DLL void openUserProfile(const PlayerAccountData &player);
GLOBED_DLL void openModPanel(int accountId = 0);

SessionId _getWarpContext();
void _clearWarpContext();

} // namespace globed
