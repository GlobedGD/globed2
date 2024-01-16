#pragma once
#include <defs.hpp>
#include <functional>

namespace util::misc {
    // Utility function for converting various enums between each other, is specialized separately for different enums
    template <typename To, typename From>
    To convertEnum(From value);

    // On first call, simply calls `func`. On repeated calls, given the same `key`, the function will not be called again. Not thread safe.
    void callOnce(const char* key, std::function<void()> func);

    // Same as `callOnce` but guaranteed to be thread-safe. Though, if another place ever calls the non-safe version, no guarantees are made.
    // When using this, it is recommended that the function does not take long to execute, due to the simplicity of the implementation.
    void callOnceSync(const char* key, std::function<void()> func);

    // Used for doing something once. `Fuse<unique-number>::tripped()` is guaranteed to evaluate to true only once. (no guarantees if accessed from multiple threads)
    template <uint32_t FuseId>
    class Fuse {
        static bool tripped() {
            static bool trip = false;
            if (!trip) {
                trip = true;
                return false;
            }

            return true;
        }
    };
}