#pragma once
#include <chrono>

namespace chrono = std::chrono;

namespace util::time {
    inline chrono::system_clock::time_point now() {
        return chrono::system_clock::now();
    }

    inline chrono::milliseconds nowMillis() {
        return chrono::duration_cast<chrono::milliseconds>(now().time_since_epoch());
    }

    inline chrono::microseconds nowMicros() {
        return chrono::duration_cast<chrono::microseconds>(now().time_since_epoch());
    }

    std::string nowPretty();
}