#pragma once

#include <stdint.h>
#include <vector>
#include <variant>

#ifdef GLOBED_BUILD
# include <dbuf/ByteReader.hpp>
# include <dbuf/ByteWriter.hpp>

# include <modules/scripting/data/SpawnData.hpp>
# include <modules/scripting/data/SetItemData.hpp>
# include <modules/scripting/data/FollowPlayerData.hpp>
# include <modules/scripting/data/MoveGroupData.hpp>
#endif

// TODO: events are not api or abi safe so this should be internal?

namespace globed {

// Event type values
// [0; 0xf000) (61440) are reserved for scripting
// [0xf000; 0xff00) (total 3840) are reserved for globed events
// 0xff00 is reserved for custom events implementation
// rest are reserved for future use

constexpr uint16_t EVENT_GLOBED_BASE = 0xf000;
constexpr uint16_t EVENT_COUNTER_CHANGE = 0xf001;
constexpr uint16_t EVENT_DISPLAY_DATA_REFRESHED = 0xf004;

constexpr uint16_t EVENT_SCR_SPAWN_GROUP = 0xf010;
constexpr uint16_t EVENT_SCR_SET_ITEM = 0xf011;
constexpr uint16_t EVENT_SCR_REQUEST_SCRIPT_LOGS = 0xf012;
constexpr uint16_t EVENT_SCR_MOVE_GROUP = 0xf013;
constexpr uint16_t EVENT_SCR_FOLLOW_PLAYER = 0xf015;
constexpr uint16_t EVENT_SCR_FOLLOW_ROTATION = 0xf016;
constexpr uint16_t EVENT_SCR_FOLLOW_ABSOLUTE = 0xf017;

constexpr uint16_t EVENT_2P_LINK_REQUEST = 0xf100;
constexpr uint16_t EVENT_2P_UNLINK = 0xf101;

constexpr uint16_t EVENT_SWITCHEROO_FULLSTATE = 0xf120;
constexpr uint16_t EVENT_SWITCHEROO_SWITCH = 0xf121;

struct UnknownEvent {
    uint16_t type;
    std::vector<uint8_t> rawData;

#ifdef GLOBED_BUILD
    Result<> encode(dbuf::ByteWriter<>& writer);
#endif
};

// In n out events

#ifdef GLOBED_BUILD

struct CounterChangeEvent {
    uint8_t rawType;
    uint32_t itemId;
    uint32_t rawValue;

    static Result<CounterChangeEvent> decode(dbuf::ByteReader<>& reader);
    Result<> encode(dbuf::ByteWriter<>& writer);
};

struct TwoPlayerLinkRequestEvent {
    int playerId;
    bool player1;

    static Result<TwoPlayerLinkRequestEvent> decode(dbuf::ByteReader<>& reader);
    Result<> encode(dbuf::ByteWriter<>& writer);
};

struct TwoPlayerUnlinkEvent {
    int playerId;

    static Result<TwoPlayerUnlinkEvent> decode(dbuf::ByteReader<>& reader);
    Result<> encode(dbuf::ByteWriter<>& writer);
};

// Incoming events

struct DisplayDataRefreshed {
    int playerId;

    static Result<DisplayDataRefreshed> decode(dbuf::ByteReader<>& reader);
};

struct SpawnGroupEvent {
    SpawnData data;

    static Result<SpawnGroupEvent> decode(dbuf::ByteReader<>& reader);
};

struct SetItemEvent {
    SetItemData data;

    static Result<SetItemEvent> decode(dbuf::ByteReader<>& reader);
};

struct MoveGroupEvent {
    MoveGroupData data;

    static Result<MoveGroupEvent> decode(dbuf::ByteReader<>& reader);
};

struct FollowPlayerEvent {
    FollowPlayerData data;

    static Result<FollowPlayerEvent> decode(dbuf::ByteReader<>& reader);
};

struct FollowAbsoluteEvent {
    FollowAbsoluteData data;

    static Result<FollowAbsoluteEvent> decode(dbuf::ByteReader<>& reader);
};

struct FollowRotationEvent {
    FollowRotationData data;

    static Result<FollowRotationEvent> decode(dbuf::ByteReader<>& reader);
};

struct SwitcherooFullStateEvent {
    int activePlayer = 0;
    bool gameActive = false;
    bool playerIndication = false;
    bool restarting = false;

    static Result<SwitcherooFullStateEvent> decode(dbuf::ByteReader<>& reader);
    Result<> encode(dbuf::ByteWriter<>& writer);
};

struct SwitcherooSwitchEvent {
    int playerId;
    enum Type : uint8_t {
        Switch = 0,
        Warning = 1,
    } type;

    static Result<SwitcherooSwitchEvent> decode(dbuf::ByteReader<>& reader);
    Result<> encode(dbuf::ByteWriter<>& writer);
};

#endif

struct InEvent {
    using Kind = std::variant<
        UnknownEvent
#ifdef GLOBED_BUILD
        ,CounterChangeEvent,
        DisplayDataRefreshed,
        SpawnGroupEvent,
        SetItemEvent,
        MoveGroupEvent,
        FollowPlayerEvent,
        FollowAbsoluteEvent,
        FollowRotationEvent,
        TwoPlayerLinkRequestEvent,
        TwoPlayerUnlinkEvent,
        SwitcherooSwitchEvent,
        SwitcherooFullStateEvent
#endif
    >;

    Kind m_kind;

#ifdef GLOBED_BUILD
    static Result<InEvent> decode(dbuf::ByteReader<>& reader);
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

    Result<> encode(dbuf::ByteWriter<>& writer);
};

struct RequestScriptLogsEvent {
    Result<> encode(dbuf::ByteWriter<>& writer);
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

    Result<> encode(dbuf::ByteWriter<>& writer);
#endif
};

}