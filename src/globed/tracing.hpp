#pragma once

#include <fmt/format.h>
#include <Geode/loader/Log.hpp>

#define TRACE(...) if (globed::_traceEnabled()) globed::trace("[T] " __VA_ARGS__)

namespace globed {
    bool _traceEnabled();

    template <typename... Args>
    void trace(geode::log::impl::FmtStr<Args...> str, Args&&... args) {
        if (_traceEnabled()) {
            geode::log::debug(str, std::forward<Args>(args)...);
        }
    }
}
