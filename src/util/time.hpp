#pragma once
#include <chrono>

namespace chrono = std::chrono;

namespace util::time {
    using clock = chrono::high_resolution_clock;
    using sysclock = chrono::system_clock;
    using time_point = clock::time_point;
    using system_time_point = sysclock::time_point;

    using days = chrono::days;
    using hours = chrono::hours;
    using minutes = chrono::minutes;
    using seconds = chrono::seconds;
    using millis = chrono::milliseconds;
    using micros = chrono::microseconds;

    template <typename Rep, typename Period>
    using duration = chrono::duration<Rep, Period>;

    inline time_point now() {
        return clock::now();
    }

    inline system_time_point systemNow() {
        return sysclock::now();
    }

    template <typename Dest, typename Rep, typename Period>
    inline Dest as(duration<Rep, Period> dur) {
        return chrono::duration_cast<Dest>(dur);
    }

    template <typename Rep, typename Period>
    inline long long asSeconds(duration<Rep, Period> tp) {
        return as<seconds>(tp).count();
    }

    template <typename Rep, typename Period>
    inline long long asMillis(duration<Rep, Period> tp) {
        return as<millis>(tp).count();
    }

    template <typename Rep, typename Period>
    inline long long asMicros(duration<Rep, Period> tp) {
        return as<micros>(tp).count();
    }

    inline millis sinceEpoch() {
        return as<millis>(now().time_since_epoch());
    }

    inline micros sinceEpochPrecise() {
        return as<micros>(now().time_since_epoch());
    }

    std::string nowPretty();
}
