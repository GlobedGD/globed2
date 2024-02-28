#pragma once
#include <unordered_map>
#include <optional>

class PlayerStore {
public:
    struct Entry {
        int32_t attempts;
        uint32_t localBest;

        bool operator==(const Entry& other) const = default;
    };

    void insertOrUpdate(int playerId, int32_t attempts, uint32_t localBest);
    void removePlayer(int playerId);
    std::optional<Entry> get(int playerId);
    std::unordered_map<int, Entry>& getAll();

private:
    std::unordered_map<int, Entry> _data;
};
