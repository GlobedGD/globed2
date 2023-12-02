#pragma once
#include <chrono>

namespace chrono = std::chrono;

namespace util::time {
    using clock = chrono::system_clock;
    using time_point = chrono::system_clock::time_point;

    using secs = chrono::seconds;
    using millis = chrono::milliseconds;
    using micros = chrono::microseconds;

    template <typename Rep, typename Period>
    using duration = chrono::duration<Rep, Period>;

    inline time_point now() {
        return clock::now();
    }

    template <typename Rep, typename Period>
    inline long long asMillis(duration<Rep, Period> tp) {
        return chrono::duration_cast<millis>(tp).count();
    }

    template <typename Rep, typename Period>
    inline long long asMicros(duration<Rep, Period> tp) {
        return chrono::duration_cast<micros>(tp).count();
    }

    inline millis sinceEpoch() {
        return chrono::duration_cast<millis>(now().time_since_epoch());
    }

    inline micros sinceEpochPrecise() {
        return chrono::duration_cast<micros>(now().time_since_epoch());
    }

    std::string nowPretty();
}