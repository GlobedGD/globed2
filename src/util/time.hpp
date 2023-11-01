#pragma once
#include <chrono>

namespace util::time {
    inline std::chrono::milliseconds now() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
    }

    inline std::chrono::microseconds nowMicros() {
        return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch());
    }

    std::string toString(std::chrono::microseconds time);
}