#pragma once

#include <fmt/format.h>
#include <Geode/loader/Log.hpp>

#define TRACE(...) if (globed::_traceEnabled()) globed::trace("[T] " __VA_ARGS__)

namespace globed {
    bool _traceEnabled();

    template <typename... Args>
    void trace(Args&&... args) {
        if (_traceEnabled()) {
            geode::log::debug(std::move(args)...);
        }
    }
}
