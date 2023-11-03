#pragma once
#include <defs.hpp>
#include <chrono>
#include <unordered_map>

namespace util::debugging {
    // To use the benchmarker, create a `Benchmarker` object, call start(id), then run all the code you want to benchmark, and call end(id).
    // It will return the number of microseconds the code took to run.
    class Benchmarker {
    public:
        Benchmarker() {}
        inline void start(std::string id) {
            _entries[id] = std::chrono::high_resolution_clock::now();
        }

        std::chrono::microseconds end(std::string id);
    private:
        std::unordered_map<std::string, std::chrono::high_resolution_clock::time_point> _entries;
    };

    std::string hexDumpAddress(uintptr_t addr, size_t bytes);
    std::string hexDumpAddress(void* ptr, size_t bytes);

#if GLOBED_CAN_USE_SOURCE_LOCATION
    std::string sourceLocation(const std::source_location loc = GLOBED_SOURCE);
    // crash the program immediately, print the location of the caller
    [[noreturn]] void suicide(const std::source_location loc = GLOBED_SOURCE);
#else
    std::string sourceLocation();
    // crash the program immediately
    [[noreturn]] void suicide();
#endif
}
