// This is a public header for the library. It can be distributed freely,
// and is intended to be included by the user of the library.

#pragma once
#include <Geode/loader/Log.hpp>
#include <Geode/platform/platform.hpp>
#include <stddef.h>
#include <stdint.h>
#include <string_view>

#define BB_VERSION "1.0.0"

struct WorkData {
    int v1;
    char *v2;
    size_t v3;
    const char *v4;
    size_t v5;
    const char *v6;
    size_t v7;
};

#ifdef GLOBED_OSS_BUILD
inline bool bb_init(const char *v = BB_VERSION)
{
    return false;
}

inline size_t bb_work(const WorkData &data)
{
    return 0;
}

#else

inline void _bblogFunc(const uint8_t *ptr, size_t size)
{
    std::string_view view{(const char *)ptr, size};
    geode::log::info("{}", view);
}

extern "C" {
bool _bb_init(const char *v, size_t base, void (*)(const uint8_t *, size_t));
size_t _bb_work(const WorkData *data);
}

inline bool bb_init(const char *v = BB_VERSION)
{
    return _bb_init(v, geode::base::get(), &_bblogFunc);
}

inline size_t bb_work(const WorkData &data)
{
    return _bb_work(&data);
}

#endif
