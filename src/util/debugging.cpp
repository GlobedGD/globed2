#include "debugging.hpp"
#include <string>

namespace util::debugging {
    std::chrono::microseconds Benchmarker::end(std::string id) {
        auto taken = std::chrono::high_resolution_clock::now() - _entries[id];
        auto micros = std::chrono::duration_cast<std::chrono::microseconds>(taken);
        _entries.erase(id);

        return micros;
    }

    std::string hexDumpAddress(uintptr_t addr, size_t bytes) {
        unsigned char* ptr = reinterpret_cast<unsigned char*>(addr);

        std::stringstream ss;

        for (size_t i = 0; i < bytes; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(i[ptr]);
        }

        return ss.str();
    }

    std::string hexDumpAddress(void* ptr, size_t bytes) {
        return hexDumpAddress(reinterpret_cast<uintptr_t>(ptr), bytes);
    }

#if GLOBED_CAN_USE_SOURCE_LOCATION
    std::string sourceLocation(const std::source_location loc) {
        return
            std::string(loc.file_name())
            + ":"
            + std::to_string(loc.line())
            + " ("
            + std::string(loc.function_name())
            + ")";
    }
#else
    std::string sourceLocation() {
        return "unknown file sorry";
    }
#endif
}