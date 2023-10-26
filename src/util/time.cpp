#include "time.hpp"

namespace util::time {
    std::string toString(std::chrono::microseconds time) {
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(time).count();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(time).count();
        auto micros = time.count();

        if (seconds > 0) {
            return std::to_string(seconds) + "." + std::to_string(millis % 1000) + "s";
        } else if (millis > 0) {
            return std::to_string(millis) + "." + std::to_string(micros % 1000) + "ms";
        } else {
            return std::to_string(micros) + "μs";
        }
    }
}