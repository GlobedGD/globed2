#pragma once

namespace globed {

constexpr uint16_t EVENT_GLOBED_BASE = 0xf000;
constexpr uint16_t EVENT_COUNTER_CHANGE = EVENT_GLOBED_BASE + 1;

struct Event {
    uint16_t type;
    std::vector<uint8_t> data;
};

}