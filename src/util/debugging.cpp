#include "debugging.hpp"

namespace util::debugging {
    std::chrono::microseconds Benchmarker::end(std::string id) {
        auto taken = std::chrono::high_resolution_clock::now() - _entries[id];
        auto micros = std::chrono::duration_cast<std::chrono::microseconds>(taken);
        _entries.erase(id);

        return micros;
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