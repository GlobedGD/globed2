#pragma once
#include <config.hpp>
#include "platform.hpp"

/*
* GLOBED_CRASH_SIGSEGV - crash with UB
* GLOBED_CRASH_SIGTRAP - crash with a breakpoint
* GLOBED_CRASH_SIGILL - crash with illegal instruction (ud2), on msvc x64 sigtrap
* so many fun ways to crash!
*
* the while(1) is inserted so compiler doesn't complain about [[noreturn]] function returning
*/

#define GLOBED_CRASH_SIGSEGV while(1) { *((volatile int*)0x69) = 0x69; }

#ifdef GLOBED_IS_UNIX
# include <signal.h>
# define GLOBED_CRASH_SIGTRAP while(1) { raise(SIGTRAP); }
#else
# include <intrin.h>
# define GLOBED_CRASH_SIGTRAP while(1) { __debugbreak(); }
#endif


#if defined(__clang__) || defined(__GNUC__)
# define GLOBED_CRASH_SIGILL while(1) { __builtin_trap(); }
#elif defined(GEODE_IS_WINDOWS) && UINTPTR_MAX < 0xffffffff // msvc x86
# define GLOBED_CRASH_SIGILL while (1) { __asm ud2; }
#else // msvc x64 has no inline asm and no __builtin_trap() so we cant do much
# define GLOBED_CRASH_SIGILL GLOBED_CRASH_SIGTRAP
#endif

#define GLOBED_SUICIDE GLOBED_CRASH_SIGTRAP

namespace globed {
    [[noreturn]] static inline void unreachable() {
#if defined(_MSC_VER) && !defined(__clang__) // MSVC
        __assume(false);
#else // GCC, Clang
        __builtin_unreachable();
#endif
    }
}