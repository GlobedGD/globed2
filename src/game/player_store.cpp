#include "player_store.hpp"

void PlayerStore::insertOrUpdate(int playerId, int32_t attempts, uint16_t percentage) {
    auto& entry = _data[playerId] = Entry {
        .attempts = attempts,
        .percentage = percentage
    };
}

std::optional<PlayerStore::Entry> PlayerStore::get(int playerId) {
    return _data.contains(playerId) ? std::nullopt : std::optional(_data.at(playerId));
}

std::unordered_map<int, PlayerStore::Entry>& PlayerStore::getAll() {
    return _data;
}
