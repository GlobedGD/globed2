#include "EventEncoder.hpp"
#include <asp/iter.hpp>

using namespace geode::prelude;

static constexpr uint32_t CENTRAL_BUILTINS_VERSION = 1;
static constexpr std::array CENTRAL_BUILTINS {
    ""
};

static constexpr uint32_t GAME_BUILTINS_VERSION = 1;
static constexpr std::array GAME_BUILTINS {
    ""
};

namespace globed {

EventEncoder::EventEncoder() {}

void EventEncoder::registerEvent(std::string name) {
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
        auto modId = slash == ev.npos ? "" : ev.substr(0, slash);
        auto remainder = ev.substr(slash + 1);

        splat[std:::string{modId}]
    }

    size_t totalEvents = events.size() + builtins.size();
    // builtins are the first ids, custom events go after them
    uint32_t currentId = builtins.size();

    qn::HeapByteWriter buf;

    return EventDictionary {
        .builtinsVersion = BUILTINS_VERSION,
        .data = buf.toVector(),
    };
}

}
