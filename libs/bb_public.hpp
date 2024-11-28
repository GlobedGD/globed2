// This is a public header for the library. It can be distributed freely,
// and is intended to be included by the user of the library.

#pragma once
#include <stddef.h>

#define BB_VERSION "0.1.0"

#ifdef GLOBED_OSS_BUILD
inline bool bb_init(const char* v = BB_VERSION) {
    return false;
}

inline size_t bb_work(const char* s, char* o, size_t ol) {
    return 0;
}

#else

extern "C" {
    bool bb_init(const char* v = BB_VERSION);
    size_t bb_work(const char* s, char* o, size_t ol);
}

#endif
