#pragma once

#include <Geode/loader/Event.hpp>

namespace globed {

struct SetupLevelIdEvent : public geode::SimpleEvent<SetupLevelIdEvent, GJGameLevel*, int64_t*> {
    using SimpleEvent::SimpleEvent;
};

}
