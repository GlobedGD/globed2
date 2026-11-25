#pragma once

#include <Geode/loader/Event.hpp>
#include "../core/SessionId.hpp"

namespace globed {

struct [[deprecated("use ResolveLevelIdEvent")]] SetupLevelIdEvent : geode::Event<SetupLevelIdEvent, bool(GJGameLevel*, int64_t*)> {
    using Event::Event;
};

struct ResolveLevelIdEvent : geode::Event<ResolveLevelIdEvent, bool(GJGameLevel*, LevelId*)> {
    using Event::Event;
};

}
