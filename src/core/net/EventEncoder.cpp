#include "EventEncoder.hpp"
#include <asp/iter.hpp>

/// Event dictionary encoding:
/// u32 builtinsVersion
/// u32 totalEvents
/// for each mod:
/// - StringVar id
/// - varuint count
/// - for each event:
/// - - StringVar name

using namespace geode::prelude;

static constexpr uint32_t CENTRAL_BUILTINS_VERSION = 1;
static constexpr std::array CENTRAL_BUILTINS {
    "test"_spr
};

static constexpr uint32_t GAME_BUILTINS_VERSION = 1;
static constexpr std::array GAME_BUILTINS {
    "test"_spr
};

namespace globed {

EventEncoder::EventEncoder() {}

void EventEncoder::registerEvent(std::string name) {
    if (name.size() < 3) return;

    auto slash = name.find('/');
    if (slash == name.npos || slash == 0 || slash == name.size() - 1) {
        return;
    }

    m_events.emplace(std::move(name));
}

EventDictionary EventEncoder::finalize(bool game) const {
    auto& bver = game ? GAME_BUILTINS_VERSION : CENTRAL_BUILTINS_VERSION;
    auto& builtins = game ? GAME_BUILTINS : CENTRAL_BUILTINS;

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
    uint32_t currentId = 0;

    // builtins are the first ids, custom events go after them
    for (auto& builtin : builtins) {
        out.mapping[currentId++] = builtin;
    }

    qn::HeapByteWriter buf;
    buf.writeU32(bver);
    buf.writeU32(totalEvents);

    for (auto& [modId, events] : splat) {
        buf.writeStringVar(modId);
        buf.writeVarUint(events.size()).unwrap();
        for (const auto& event : events) {
            buf.writeStringVar(event);
            out.mapping[currentId++] = asp::BoxedString::format("{}/{}", modId, event);
        }
    }

    out.data = std::move(buf).intoVector();
    return out;
}

}
