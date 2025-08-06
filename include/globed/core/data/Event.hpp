#pragma once

namespace globed {

// Event type values
// [0; 0xf000) (61440) are reserved for scripting
// [0xf000; 0xf800) (total 2048) are reserved for globed events
// [0xf800; 0xffff] (total 2048) are reserved for custom mod events

// Globed events
// 0xf001 - EVENT_COUNTER_CHANGE
// 0xf010 - EVENT_SPAWN_GROUP
// 0xf011 - EVENT_SET_ITEM

constexpr uint16_t EVENT_GLOBED_BASE = 0xf000;
constexpr uint16_t EVENT_COUNTER_CHANGE = 0xf001;
constexpr uint16_t EVENT_SPAWN_GROUP = 0xf010;
constexpr uint16_t EVENT_SET_ITEM = 0xf011;
constexpr uint16_t EVENT_CUSTOM_BASE = 0xf800;

struct Event {
    uint16_t type;
    std::vector<uint8_t> data;
};

}