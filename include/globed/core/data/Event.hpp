#pragma once

namespace globed {

constexpr uint16_t EVENT_COUNTER_CHANGE = 1;

struct Event {
    uint16_t type;
    std::vector<uint8_t> data;
};

}