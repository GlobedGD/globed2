#include "EventEncoder.hpp"

/// Event dictionary encoding:
/// u32 builtinsVersion
/// u32 totalEvents
/// for each mod:
/// - StringU8 id
/// - varuint count
/// - for each event:
/// - - StringU8 name

/// Events encoding, for the event buffer in messages, which may contain many events
/// varuint count
/// for each event:
/// - [u8/u16/u32] id (type dependant on total event count in the dictionary)
/// - u8 flags
/// - [optional] varuint amount of player ids, if TARGET_PLAYERS flag is set
/// - [optional] array of i32 player ids, if TARGET_PLAYERS flag is set
/// - [optional] i32 player id, sender of the event, if SENT_BY_PLAYER flag is set
/// - [optional] varuint length of the data, unless the NO_DATA flag is set
/// - [optional] blob data

using namespace geode::prelude;

static constexpr uint32_t CENTRAL_BUILTINS_VERSION = 1;
static constexpr std::array CENTRAL_BUILTINS {
    "test"_spr
};

static constexpr uint32_t GAME_BUILTINS_VERSION = 1;
static constexpr std::array GAME_BUILTINS {
    "globed/counter-change",
    "globed/display-data-refreshed",

    "globed/scripting.custom",
    "globed/scripting.spawn-group",
    "globed/scripting.set-item",
    "globed/scripting.request-script-logs",
    "globed/scripting.move-group",
    "globed/scripting.follow-player",
    "globed/scripting.follow-rotation",
    "globed/scripting.follow-absolute",

    "globed/2p.link",
    "globed/2p.unlink",

    "globed/switcheroo.full-state",
    "globed/switcheroo.switch",
};

namespace globed {

/// Dictionary

std::optional<asp::BoxedString> EventDictionary::lookup(uint32_t id) const {
    if (id >= mapping.size()) return std::nullopt;
    return mapping[id];
}

std::optional<uint32_t> EventDictionary::lookupId(std::string_view name) const {
    return asp::iter::from(mapping).enumerate().find([&](auto& el) {
        return el.second == name;
    }).transform([](auto el) {
        return el.first;
    });
}

/// Event iterator, allows zero alloc iteration over events in a buffer

EventIterator::EventIterator(std::span<const uint8_t> data, EventDictionary& dictionary) : m_reader(data), m_dictionary(dictionary) {
    m_remCount = m_reader.readVarUint().unwrapOr(0);
}

std::optional<Result<RawBorrowedEvent>> EventIterator::next() {
    if (m_reader.remainingSize() == 0 || m_eof || m_remCount == 0) {
        if (m_remCount == 0) {
            return std::nullopt;
        } else {
            return Err("unexpected EOF, could not read {} remaining events", m_remCount);
        }
    }

    // set eof preemptively, so in case we error out the next call will be a no-op
    m_eof = true;

    size_t total = m_dictionary.mapping.size();
    uint32_t id;
    if (total < 256) {
        id = GEODE_UNWRAP(m_reader.readU8());
    } else if (total < 65536) {
        id = GEODE_UNWRAP(m_reader.readU16());
    } else {
        id = GEODE_UNWRAP(m_reader.readU32());
    }

    auto strId = m_dictionary.lookup(id);
    if (!strId) {
        return Err("unknown event ID (not in dictionary): {}", strId);
    }

    EventFlags flags = static_cast<EventFlags>(GEODE_UNWRAP(m_reader.readU8()));
    if ((flags & EventFlags::EXTENDED_FLAGS) != 0) {
        // for future use, currently unused
        uint8_t extFlags = GEODE_UNWRAP(m_reader.readU8());
    }

    RawBorrowedEvent out{};
    out.name = *strId;

    if ((flags & EventFlags::SENT_BY_PLAYER) != 0) {
        out.options.sender = GEODE_UNWRAP(m_reader.readI32());
    }

    if ((flags & EventFlags::SEND_BACK) != 0) {
        out.options.sendBack = true;
    }

    if ((flags & EventFlags::NO_DATA) == 0) {
        auto len = GEODE_UNWRAP(m_reader.readVarUint());
        auto pos = m_reader.position();
        out.data = GEODE_UNWRAP(m_reader.source().slice(pos, len));
    }

    m_eof = false;
    m_remCount--;

    return Ok(std::move(out));
}

EventEncoder::EventEncoder() {}

bool EventEncoder::registerEvent(std::string name) {
    if (name.size() < 3) return false;

    auto slash = name.find('/');
    if (slash == name.npos || slash == 0 || slash == name.size() - 1) {
        return false;
    }

    std::string_view sv{name};
    auto modId = sv.substr(0, slash);
    auto eventId = sv.substr(slash + 1);

    if (modId.size() >= 127 || eventId.size() >= 127) {
        return false;
    }

    m_events.emplace(std::move(name));
    return true;
}

EventDictionary EventEncoder::finalize(bool game) const {
    auto& bver = game ? GAME_BUILTINS_VERSION : CENTRAL_BUILTINS_VERSION;
    auto builtins = game
        ? std::span<const char* const>{GAME_BUILTINS}
        : std::span<const char* const>{CENTRAL_BUILTINS};

    // sort all events, remove builtins
    auto events = asp::iter::consume(m_events).filter([&](auto& el) {
        return std::ranges::find(builtins, el) != builtins.end();
    }).collect();
    std::ranges::sort(events);

    // as an optimization we dont encode event names one by one, we group them by mod id
    std::unordered_map<std::string, std::vector<std::string>> splat;
    for (std::string_view ev : m_events) {
        auto slash = ev.find('/');
        auto modId = ev.substr(0, slash);
        auto remainder = ev.substr(slash + 1);

        splat[std::string{modId}].push_back(std::string{remainder});
    }

    EventDictionary out{};

    size_t totalEvents = events.size() + builtins.size();

    // builtins are the first ids, custom events go after them
    for (auto& builtin : builtins) {
        out.mapping.emplace_back(builtin);
    }

    dbuf::ByteWriter buf;
    buf.writeU32(bver);
    buf.writeU32(totalEvents);

    for (auto& [modId, events] : splat) {
        buf.writeStringU8(modId);
        buf.writeVarUint(events.size()).unwrap();
        for (const auto& event : events) {
            buf.writeStringU8(event);
            out.mapping.emplace_back(asp::BoxedString::format("{}/{}", modId, event));
        }
    }

    auto written = buf.written();
    out.data = std::vector<uint8_t>{written.begin(), written.end()};
    return out;
}

}
