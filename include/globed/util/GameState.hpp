#pragma once

#include <globed/config.hpp>

namespace globed {

enum class GameState {
    Active,
    Afk,
    Closed,
};

/// Obtains the current game state.
/// Active: the game window is focused and the user is seen making inputs
/// Afk: one of the two:
/// 1. the game window is focused but the user is idle for a certain period
/// 2. the game window has been unfocused for a certain period
/// Closed: the game window is minimized, or the user has been inactive for a long period while unfocused
GLOBED_DLL GameState getCurrentGameState();

/// Returns whether the game is currently focused, only implemented on Windows and MacOS.
GLOBED_DLL bool isGameFocused();

/// Returns how many ms passed since last user input
GLOBED_DLL uint64_t timeSinceInput();

}
