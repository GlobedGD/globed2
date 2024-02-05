#include "format.hpp"
#include <cmath>
#include <iomanip>

namespace util::format {
    std::string formatDateTime(time::system_time_point tp) {
        auto timet = time::sysclock::to_time_t(tp);
        auto nowms = chrono::duration_cast<chrono::milliseconds>(tp.time_since_epoch()) % 1000;

        std::tm time_info = *std::localtime(&timet);

        std::ostringstream oss;
        oss << std::put_time(&time_info, "%Y-%m-%d %H:%M:%S") << '.' << std::setfill('0') << std::setw(3) << nowms.count();

        return oss.str();
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

    std::string formatErrorMessage(std::string message) {
        if (message.find("<!DOCTYPE html>") != std::string::npos) {
            message = "<HTML response, not showing>";
        } else if (message.size() > 128) {
            message = message.substr(0, 128) + "...";
        }

        return message;
    }
}