#pragma once
#include <defs.hpp>

class PlayerStore {
public:
    struct Entry {
        int32_t attempts;
        uint16_t percentage;

        bool operator==(const Entry& other) const = default;
    };

    void insertOrUpdate(int playerId, int32_t attempts, uint16_t percentage);
    void removePlayer(int playerId);
    std::optional<Entry> get(int playerId);
    std::unordered_map<int, Entry>& getAll();

private:
    std::unordered_map<int, Entry> _data;
};
