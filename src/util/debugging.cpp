#include "debugging.hpp"
#include <Geode/Geode.hpp>

namespace util::debugging {
    void Benchmarker::end(std::string id) {
        auto taken = std::chrono::high_resolution_clock::now() - _entries[id];
        auto micros = std::chrono::duration_cast<std::chrono::microseconds>(taken).count();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(taken).count();

        if (millis > 0) {
            geode::log::info("[B] [!] {} took {}ms {}micros to run", id, millis, micros);
        } else {
            geode::log::info("[B] {} took {}micros to run", id, micros);
        }
    }

#if GLOBED_CAN_USE_SOURCE_LOCATION
    std::string sourceLocation(const std::source_location loc) {
        return fmt::format("{}:{} ({})", loc.file_name(), loc.line(), loc.function_name());
    }
#else
    std::string sourceLocation() {
        return "unknown file sorry";
    }
#endif
}