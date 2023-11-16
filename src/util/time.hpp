#pragma once
#include <chrono>

namespace util::time {
    namespace chrono = std::chrono;

    inline chrono::system_clock::time_point now() {
        return chrono::system_clock::now();
    }

    inline chrono::milliseconds nowMillis() {
        return chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch());
    }

    inline chrono::microseconds nowMicros() {
        return chrono::duration_cast<chrono::microseconds>(chrono::system_clock::now().time_since_epoch());
    }

    std::string nowPretty();
}