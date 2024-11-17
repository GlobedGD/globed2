// This is a public header for the library. It can be distributed freely,
// and is intended to be included by the user of the library.

#pragma once

#ifdef _WIN32
# ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
# endif
# include <Windows.h>
#else
# include <dlfcn.h>
#endif

#define BB_VERSION "1.0.0"

namespace _bb {
    static inline const char* dn = "blank";

    template <typename T>
    T* getExportedFunction(const char* name) {
        static auto hModule = []{
#ifdef _WIN32
            return GetModuleHandleA(dn);
#else
            return dlopen(dn, RTLD_LAZY);
#endif
        }();

        if (!hModule) {
            return nullptr;
        }

#ifdef _WIN32
        return reinterpret_cast<T*>(GetProcAddress(hModule, name));
#else
        return reinterpret_cast<T*>(dlsym(hModule, name));
#endif
    }
}

extern "C" {
    // Returns true if the library was initialized successfully, false if it failed or is unavailable (OSS build)
    inline bool bb_init(const char* dn, const char* v = BB_VERSION) {
        ::_bb::dn = dn;
        auto f = _bb::getExportedFunction<bool(const char*)>("bb_init");
        return f ? f(v) : false;
    }

    // returns false on failure, true on success
    inline size_t bb_work(const char* s, char* o, size_t ol) {
        auto f = _bb::getExportedFunction<size_t (const char*, char*, size_t)>("bb_work");
        return f ? f(s, o, ol) : 0;
    }
}
