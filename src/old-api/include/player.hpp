#pragma once

#include "_internal.hpp"
#include <Geode/binding/PlayerObject.hpp>

class PlayerObject;

namespace globed::player {
    // Returns whether the given `PlayerObject` belongs to an online player.
    bool isGlobedPlayer(PlayerObject* node);

    // Returns whether the given `PlayerObject` belongs to an online player.
    // Is significantly faster than `isGlobedPlayer`, but may report false positives or false negatives, as it relies on the tag value on the player object.
    // Only use this function if `isGlobedPlayer` is an actual performance concern.
    bool isGlobedPlayerFast(PlayerObject* node);

    // Explodes a random player in the level. I will not elaborate further.
    Result<void> explodeRandomPlayer();

    // Returns the account ID of a player by their `PlayerObject`.
    // If the given object does not represent an online player, -1 is returned.
    Result<int> accountIdForPlayer(PlayerObject* node);

    // Returns the amount of players on the current level.
    // If the player is not in a level, returns an error.
    Result<size_t> playersOnLevel();

    // Returns the amount of players online.
    // If the player is not connected to a server, returns an error.
    Result<size_t> playersOnline();
} // namespace globed::player

// Implementation

namespace globed::player {
    inline bool isGlobedPlayer(PlayerObject* node) {
        return _internal::request<bool>(_internal::Type::IsGlobedPlayer, node).unwrapOr(false);
    }

    inline bool isGlobedPlayerFast(PlayerObject* node) {
        // Yep.
        return node->getTag() == 3458738;
    }

    inline Result<void> explodeRandomPlayer() {
        return _internal::requestVoid(_internal::Type::ExplodeRandomPlayer);
    }

    inline Result<int> accountIdForPlayer(PlayerObject* node) {
        return _internal::request<int>(_internal::Type::AccountIdForPlayer, node);
    }

    inline Result<size_t> playersOnLevel() {
        return _internal::request<size_t>(_internal::Type::PlayersOnLevel);
    }

    inline Result<size_t> playersOnline() {
        return _internal::request<size_t>(_internal::Type::PlayersOnline);
    }

    inline Result<std::vector<int>> getAllPlayerIds() {
        return _internal::request<std::vector<int>>(_internal::Type::AllPlayerIds);
    }

    inline Result<std::pair<PlayerObject*, PlayerObject*>> getPlayerObjectsForId(int accountId) {
        return _internal::request<std::pair<PlayerObject*, PlayerObject*>, int>(_internal::Type::PlayerObjectsForId, accountId);
    }
} // namespace globed::player
