#pragma once

#include "_internal.hpp"

namespace globed::callbacks {
    using PlayerJoinFn = std::function<void (int, PlayerObject*, PlayerObject*)>;
    using PlayerLeaveFn = std::function<void (int, PlayerObject*, PlayerObject*)>;

    // Sets a callback that will be called when another player joins the current level.
    // Callback is removed when the user leaves the level.
    Result<void> onPlayerJoin(PlayerJoinFn func);

    // Sets a callback that will be called when another player leaves the current level.
    // Callback is removed when the user leaves the level.
    Result<void> onPlayerLeave(PlayerLeaveFn func);
} // namespace globed::callbacks

// Implementation

namespace globed::callbacks {
    inline Result<void> onPlayerJoin(PlayerJoinFn func) {
        return _internal::request<void>(_internal::Type::CbPlayerJoin);
    }

    inline Result<void> onPlayerLeave(PlayerLeaveFn func) {
        return _internal::request<void>(_internal::Type::CbPlayerLeave);
    }
} // namespace globed::callbacks
