#pragma once
#include <globed/core/SessionId.hpp>
#include <globed/core/data/RoomPlayer.hpp>
#include <globed/config.hpp>
#include <globed/util/CCData.hpp>
#include <asp/time/Instant.hpp>

namespace globed {

enum class WarpSource {
    /// Pressing the join button on a user
    Manual,
    /// Getting warped by the room owner
    Room,
    /// Getting warped by the server administrator
    Global,
};

struct WarpContext {
    SessionId session;
    WarpSource source;
};

GLOBED_DLL void warpToSession(WarpContext context);

GLOBED_DLL void openUserProfile(int accountId, int userId = 0, std::string_view username = "");
GLOBED_DLL void openUserProfile(const RoomPlayer& player);
GLOBED_DLL void openUserProfile(const PlayerAccountData& player);
GLOBED_DLL void openModPanel(int accountId = 0);

}
