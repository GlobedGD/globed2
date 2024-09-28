#include "time.hpp"
#include <iomanip>
#include <sstream>

#include <fmt/chrono.h>

namespace util::time {
    std::string nowPretty() {
        auto time = chrono::system_clock::now();
        std::time_t curTime = chrono::system_clock::to_time_t(time);

        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(time.time_since_epoch()).count() % 1000;

        return fmt::format("{:%H:%M:%S}.{:03}", fmt::localtime(curTime), millis);
    }

    bool isAprilFools() {
        auto now = std::chrono::system_clock::now();
        auto timeNow = std::chrono::system_clock::to_time_t(now);
        struct tm* tm_local = std::localtime(&timeNow);

        return tm_local->tm_mon == 3 && tm_local->tm_mday == 1;
    }
}
