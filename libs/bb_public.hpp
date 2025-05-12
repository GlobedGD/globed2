// This is a public header for the library. It can be distributed freely,
// and is intended to be included by the user of the library.

#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string_view>
#include <Geode/platform/platform.hpp>
#include <Geode/loader/Log.hpp>

#define BB_VERSION "0.3.0"

#ifdef GLOBED_OSS_BUILD
inline bool bb_init(const char* v = BB_VERSION) {
    return false;
}

inline size_t bb_work(const char* s, char* o, size_t ol) {
    return 0;
}

#else

inline void _bblogFunc(const uint8_t* ptr, size_t size) {
    std::string_view view{(const char*)ptr, size};
    geode::log::debug("{}", view);
}

extern "C" {
    bool _bb_init(const char* v, size_t base, void (*)(const uint8_t*, size_t));
    size_t _bb_work(const char* s, char* o, size_t ol);
}

inline bool bb_init(const char* v = BB_VERSION) {
    return _bb_init(v, geode::base::get(), &_bblogFunc);
}

inline size_t bb_work(const char* s, char* o, size_t ol) {
    return _bb_work(s, o, ol);
}

#endif
