#pragma once
#include <defs.hpp>

namespace util::formatting {
    // example: 2.123s, 69.123ms
    std::string formatTime(std::chrono::microseconds time);

    // example: 123.4KiB
    std::string formatBytes(uint64_t bytes);
}