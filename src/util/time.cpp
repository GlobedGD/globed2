#include "time.hpp"
#include <iomanip>
#include <sstream>

#include <fmt/chrono.h>
#include <asp/time/SystemTime.hpp>

using namespace asp::time;

namespace util::time {
    std::string nowPretty() {
        auto time = SystemTime::now();
        auto curTime = time.to_time_t();

        auto millis = time.timeSinceEpoch().subsecMillis();

        return fmt::format("{:%H:%M:%S}.{:03}", fmt::localtime(curTime), millis);
    }

    bool isAprilFools() {
        auto now = std::chrono::system_clock::now();
        auto timeNow = std::chrono::system_clock::to_time_t(now);
        struct tm tm_local = fmt::localtime(timeNow);

        return tm_local.tm_mon == 3 && tm_local.tm_mday == 1;
    }
}
