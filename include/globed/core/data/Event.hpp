#pragma once

#include <qunet/buffers/ByteReader.hpp>
#include <qunet/buffers/HeapByteWriter.hpp>

#include <modules/scripting/data/SpawnData.hpp>
#include <modules/scripting/data/SetItemData.hpp>
#include <modules/scripting/data/FollowPlayerData.hpp>
#include <modules/scripting/data/MoveGroupData.hpp>

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

// In n out events

struct CounterChangeEvent {
    uint8_t rawType;
    uint32_t itemId;
    uint32_t rawValue;

    static geode::Result<CounterChangeEvent> decode(qn::ByteReader& reader);
    geode::Result<> encode(qn::HeapByteWriter& writer);
};

struct TwoPlayerLinkRequestEvent {
    int playerId;
    bool player1;

    static geode::Result<TwoPlayerLinkRequestEvent> decode(qn::ByteReader& reader);
    geode::Result<> encode(qn::HeapByteWriter& writer);
};

struct TwoPlayerUnlinkEvent {
    int playerId;

    static geode::Result<TwoPlayerUnlinkEvent> decode(qn::ByteReader& reader);
    geode::Result<> encode(qn::HeapByteWriter& writer);
};

// Incoming events

struct SpawnGroupEvent {
    SpawnData data;

    static geode::Result<SpawnGroupEvent> decode(qn::ByteReader& reader);
};

struct SetItemEvent {
    SetItemData data;

    static geode::Result<SetItemEvent> decode(qn::ByteReader& reader);
};

struct MoveGroupEvent {
    MoveGroupData data;

    static geode::Result<MoveGroupEvent> decode(qn::ByteReader& reader);
};

struct MoveGroupAbsoluteEvent {
    MoveAbsGroupData data;

    static geode::Result<MoveGroupAbsoluteEvent> decode(qn::ByteReader& reader);
};

struct FollowPlayerEvent {
    FollowPlayerData data;

    static geode::Result<FollowPlayerEvent> decode(qn::ByteReader& reader);
};

struct InEvent {
    using Kind = std::variant<
        CounterChangeEvent,
        SpawnGroupEvent,
        SetItemEvent,
        MoveGroupEvent,
        MoveGroupAbsoluteEvent,
        FollowPlayerEvent,
        TwoPlayerLinkRequestEvent,
        TwoPlayerUnlinkEvent
    >;

    Kind m_kind;

    static geode::Result<InEvent> decode(qn::ByteReader& reader);

    template <typename T>
    bool is() const {
        return std::holds_alternative<T>(m_kind);
    }

    template <typename T>
    T& as() {
        return std::get<T>(m_kind);
    }

    template <typename T>
    const T& as() const {
        return std::get<T>(m_kind);
    }
};

// Outgoing events

struct ScriptedEvent {
    uint16_t type;
    std::vector<std::variant<int, float>> args;

    geode::Result<> encode(qn::HeapByteWriter& writer);
};

struct RequestScriptLogsEvent {
    geode::Result<> encode(qn::HeapByteWriter& writer);
};

struct OutEvent {
    using Kind = std::variant<
        CounterChangeEvent,
        TwoPlayerLinkRequestEvent,
        TwoPlayerUnlinkEvent,
        ScriptedEvent,
        RequestScriptLogsEvent
    >;

    OutEvent(Kind&& k) : m_kind(std::move(k)) {}
    OutEvent(CounterChangeEvent e) : m_kind(std::move(e)) {}
    OutEvent(TwoPlayerLinkRequestEvent e) : m_kind(std::move(e)) {}
    OutEvent(TwoPlayerUnlinkEvent e) : m_kind(std::move(e)) {}
    OutEvent(ScriptedEvent e) : m_kind(std::move(e)) {}
    OutEvent(RequestScriptLogsEvent e) : m_kind(std::move(e)) {}

    Kind m_kind;

    geode::Result<> encode(qn::HeapByteWriter& writer);
};

}