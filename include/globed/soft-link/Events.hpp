#pragma once

#include <Geode/loader/Event.hpp>

namespace globed {

struct SetupLevelIdEvent : public geode::Event<SetupLevelIdEvent, bool(GJGameLevel*, int64_t*)> {
    using Event::Event;
};

}
