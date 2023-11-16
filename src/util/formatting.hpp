#pragma once
#include <defs.hpp>

namespace util::formatting {
    namespace chrono = std::chrono;

    // example: 2.123s, 69.123ms
    std::string formatTime(chrono::microseconds time);

    // example: 2023-11-16 19:43:50.200
    std::string formatDate(chrono::system_clock::time_point tp);

    // example: 123.4KiB
    std::string formatBytes(uint64_t bytes);
}