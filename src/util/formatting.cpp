#include "formatting.hpp"
#include <cmath>
#include <iomanip>

namespace util::formatting {
    std::string formatTime(std::chrono::microseconds time) {
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

    std::string formatBytes(uint64_t bytes) {
        // i did not write this myself
        static const char* suffixes[] = {"B", "KiB", "MiB", "GiB", "TiB", "PiB", "EiB", "ZiB", "YiB"};

        if (bytes == 0) {
            return "0B";
        }

        int exp = static_cast<int>(std::log2(bytes) / 10);
        double value = static_cast<double>(bytes) / std::pow(1024, exp);

        std::ostringstream oss;

        if (std::fmod(value, 1.0) == 0) {
            oss << static_cast<int>(value);
        } else {
            oss << std::fixed << std::setprecision(1) << value;
        }

        oss << suffixes[exp];

        return oss.str();
    }
}