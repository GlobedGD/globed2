#pragma once
#include <defs.hpp>
#include <source_location>

namespace util::debugging {
    class Benchmarker {
    public:
        Benchmarker() {}
        inline void start(std::string id) {
            _entries[id] = std::chrono::high_resolution_clock::now();
        }

        void end(std::string id);
    private:
        std::unordered_map<std::string, std::chrono::high_resolution_clock::time_point> _entries;
    };

#if GLOBED_CAN_USE_SOURCE_LOCATION
    std::string sourceLocation(const std::source_location loc = GLOBED_SOURCE);
#else
    std::string sourceLocation();
#endif
}
