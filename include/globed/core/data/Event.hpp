#pragma once

#include <stdint.h>
#include <vector>
#include <variant>

#ifdef GLOBED_BUILD
# include <qunet/buffers/ByteReader.hpp>
# include <qunet/buffers/HeapByteWriter.hpp>

# include <modules/scripting/data/SpawnData.hpp>
# include <modules/scripting/data/SetItemData.hpp>
# include <modules/scripting/data/FollowPlayerData.hpp>
# include <modules/scripting/data/MoveGroupData.hpp>
#endif

namespace globed {

// Event type values
// [0; 0xf000) (61440) are reserved for scripting
// [0xf000; 0xff00) (total 3840) are reserved for globed events
// 0xff00 is reserved for custom events implementation
// rest are reserved for future use

constexpr uint16_t EVENT_GLOBED_BASE = 0xf000;
constexpr uint16_t EVENT_COUNTER_CHANGE = 0xf001;

constexpr uint16_t EVENT_SCR_SPAWN_GROUP = 0xf010;
constexpr uint16_t EVENT_SCR_SET_ITEM = 0xf011;
constexpr uint16_t EVENT_SCR_REQUEST_SCRIPT_LOGS = 0xf012;
constexpr uint16_t EVENT_SCR_MOVE_GROUP = 0xf013;
constexpr uint16_t EVENT_SCR_MOVE_GROUP_ABSOLUTE = 0xf014;
constexpr uint16_t EVENT_SCR_FOLLOW_PLAYER = 0xf015;
constexpr uint16_t EVENT_SCR_FOLLOW_ROTATION = 0xf016;

constexpr uint16_t EVENT_2P_LINK_REQUEST = 0xf100;
constexpr uint16_t EVENT_2P_UNLINK = 0xf101;

constexpr uint16_t EVENT_SWITCHEROO_FULLSTATE = 0xf120;
constexpr uint16_t EVENT_SWITCHEROO_SWITCH = 0xf121;

struct UnknownEvent {
    uint16_t type;
    std::vector<uint8_t> rawData;

#ifdef GLOBED_BUILD
    Result<> encode(qn::HeapByteWriter& writer);
#endif
};

// In n out events

#ifdef GLOBED_BUILD

struct CounterChangeEvent {
    uint8_t rawType;
    uint32_t itemId;
    uint32_t rawValue;

    static Result<CounterChangeEvent> decode(qn::ByteReader& reader);
    Result<> encode(qn::HeapByteWriter& writer);
};

struct TwoPlayerLinkRequestEvent {
    int playerId;
    bool player1;

    static Result<TwoPlayerLinkRequestEvent> decode(qn::ByteReader& reader);
    Result<> encode(qn::HeapByteWriter& writer);
};

struct TwoPlayerUnlinkEvent {
    int playerId;

    static Result<TwoPlayerUnlinkEvent> decode(qn::ByteReader& reader);
    Result<> encode(qn::HeapByteWriter& writer);
};

// Incoming events

struct SpawnGroupEvent {
    SpawnData data;

    static Result<SpawnGroupEvent> decode(qn::ByteReader& reader);
};

struct SetItemEvent {
    SetItemData data;

    static Result<SetItemEvent> decode(qn::ByteReader& reader);
};

struct MoveGroupEvent {
    MoveGroupData data;

    static Result<MoveGroupEvent> decode(qn::ByteReader& reader);
};

struct MoveGroupAbsoluteEvent {
    MoveAbsGroupData data;

    static Result<MoveGroupAbsoluteEvent> decode(qn::ByteReader& reader);
};

struct FollowPlayerEvent {
    FollowPlayerData data;

    static Result<FollowPlayerEvent> decode(qn::ByteReader& reader);
};

struct FollowRotationEvent {
    FollowRotationData data;

    static Result<FollowRotationEvent> decode(qn::ByteReader& reader);
};

struct SwitcherooFullStateEvent {
    int activePlayer = 0;
    bool gameActive = false;
    bool playerIndication = false;
    bool restarting = false;

    static Result<SwitcherooFullStateEvent> decode(qn::ByteReader& reader);
    Result<> encode(qn::HeapByteWriter& writer);
};

struct SwitcherooSwitchEvent {
    int playerId;
    enum Type : uint8_t {
        Switch = 0,
        Warning = 1,
    } type;

    static Result<SwitcherooSwitchEvent> decode(qn::ByteReader& reader);
    Result<> encode(qn::HeapByteWriter& writer);
};

#endif

struct InEvent {
    using Kind = std::variant<
        UnknownEvent
#ifdef GLOBED_BUILD
        ,CounterChangeEvent,
        SpawnGroupEvent,
        SetItemEvent,
        MoveGroupEvent,
        MoveGroupAbsoluteEvent,
        FollowPlayerEvent,
        FollowRotationEvent,
        TwoPlayerLinkRequestEvent,
        TwoPlayerUnlinkEvent,
        SwitcherooSwitchEvent,
        SwitcherooFullStateEvent
#endif
    >;

    Kind m_kind;

#ifdef GLOBED_BUILD
    static Result<InEvent> decode(qn::ByteReader& reader);
#endif

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

#ifdef GLOBED_BUILD

struct ScriptedEvent {
    uint16_t type;
    std::vector<std::variant<int, float>> args;

    Result<> encode(qn::HeapByteWriter& writer);
};

struct RequestScriptLogsEvent {
    Result<> encode(qn::HeapByteWriter& writer);
};

#endif

struct OutEvent {
    using Kind = std::variant<
        UnknownEvent
#ifdef GLOBED_BUILD
        ,CounterChangeEvent,
        TwoPlayerLinkRequestEvent,
        TwoPlayerUnlinkEvent,
        ScriptedEvent,
        RequestScriptLogsEvent,
        SwitcherooSwitchEvent,
        SwitcherooFullStateEvent
#endif
    >;

    OutEvent(Kind&& k) : m_kind(std::move(k)) {}
    OutEvent(UnknownEvent e) : m_kind(std::move(e)) {}
    Kind m_kind;

#ifdef GLOBED_BUILD
    OutEvent(CounterChangeEvent e) : m_kind(std::move(e)) {}
    OutEvent(TwoPlayerLinkRequestEvent e) : m_kind(std::move(e)) {}
    OutEvent(TwoPlayerUnlinkEvent e) : m_kind(std::move(e)) {}
    OutEvent(ScriptedEvent e) : m_kind(std::move(e)) {}
    OutEvent(RequestScriptLogsEvent e) : m_kind(std::move(e)) {}
    OutEvent(SwitcherooSwitchEvent e) : m_kind(std::move(e)) {}
    OutEvent(SwitcherooFullStateEvent e) : m_kind(std::move(e)) {}

    Result<> encode(qn::HeapByteWriter& writer);
#endif
};

}