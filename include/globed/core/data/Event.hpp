#pragma once

namespace globed {

// Event type values
// [0; 0xf000) (61440) are reserved for scripting
// [0xf000; 0xf800) (total 2048) are reserved for globed events
// [0xf800; 0xffff] (total 2048) are reserved for custom mod events

// Globed events
// 0xf001 - EVENT_COUNTER_CHANGE
// 0xf010 - EVENT_SCR_SPAWN_GROUP
// 0xf011 - EVENT_SCR_SET_ITEM
// 0xf100 - EVENT_2P_LINK_REQUEST
// 0xf101 - EVENT_2P_UNLINK

constexpr uint16_t EVENT_GLOBED_BASE = 0xf000;
constexpr uint16_t EVENT_COUNTER_CHANGE = 0xf001;

constexpr uint16_t EVENT_SCR_SPAWN_GROUP = 0xf010;
constexpr uint16_t EVENT_SCR_SET_ITEM = 0xf011;
constexpr uint16_t EVENT_SCR_REQUEST_SCRIPT_LOGS = 0xf012;
constexpr uint16_t EVENT_SCR_MOVE_GROUP = 0xf013;
constexpr uint16_t EVENT_SCR_MOVE_GROUP_ABSOLUTE = 0xf014;
constexpr uint16_t EVENT_SCR_FOLLOW_PLAYER = 0xf015;

constexpr uint16_t EVENT_2P_LINK_REQUEST = 0xf100;
constexpr uint16_t EVENT_2P_UNLINK = 0xf101;

constexpr uint16_t EVENT_CUSTOM_BASE = 0xf800;

struct Event {
    uint16_t type;
    std::vector<uint8_t> data;
};

}