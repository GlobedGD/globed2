#include "player_store.hpp"

void PlayerStore::insertOrUpdate(int playerId, int32_t attempts, uint32_t localBest) {
    auto& entry = _data[playerId] = Entry {
        .attempts = attempts,
        .localBest = localBest
    };
}

void PlayerStore::removePlayer(int playerId) {
    _data.erase(playerId);
}

std::optional<PlayerStore::Entry> PlayerStore::get(int playerId) {
    return _data.contains(playerId) ? std::nullopt : std::optional(_data.at(playerId));
}

std::unordered_map<int, PlayerStore::Entry>& PlayerStore::getAll() {
    return _data;
}
