#pragma once
#include <asp/ptr/BoxedString.hpp>
#include <span>
#include <vector>
#include <stdint.h>

namespace globed {

enum class EventServer {
    Central = 1,
    Game = 2,
    Both = 3,
};

struct EventOptions final {
    EventOptions() = default;

    EventServer server = EventServer::Central;
    int32_t sender = 0; // ONLY valid for server -> client; player id, 0 if server
    std::vector<int32_t> targetPlayers;
    bool reliable = true;
    bool urgent = false;
    bool sendBack = false;

    void* _reserved[8] = {};
};

struct RawBorrowedEvent final {
    asp::BoxedString name;
    std::span<const uint8_t> data;
    EventOptions options;
};

struct RawEvent final {
    asp::BoxedString name;
    std::vector<uint8_t> data;
    EventOptions options;
};

}
