#include "time.hpp"

namespace util::time {
    std::string nowPretty() {
        auto time = std::chrono::system_clock::now();
        std::time_t curTime = std::chrono::system_clock::to_time_t(time);
        std::tm tm = *std::localtime(&curTime);

        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(time.time_since_epoch()).count() % 1000;

        std::ostringstream ss;
        ss << std::put_time(&tm, "%H:%M:%S") << "." << std::setfill('0') << std::setw(3) << millis;

        return ss.str();
    }
}