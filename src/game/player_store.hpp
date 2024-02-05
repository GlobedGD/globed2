#pragma once
#include <defs.hpp>

class PlayerStore {
public:
    struct Entry {
        int32_t attempts;
        uint16_t percentage;
    };

    void insertOrUpdate(int playerId, int32_t attempts, uint16_t percentage);
    std::optional<Entry> get(int playerId);
    std::unordered_map<int, Entry>& getAll();

private:
    std::unordered_map<int, Entry> _data;
};
