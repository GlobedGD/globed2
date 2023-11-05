#pragma once
#include "basic.hpp"
#include "platform.hpp"

/*
* GLOBED_CRASH_SIGSEGV - crash with UB
* GLOBED_CRASH_SIGTRAP - crash with a breakpoint
* GLOBED_CRASH_SIGILL - crash with illegal instruction (ud2), on msvc x64 sigtrap
* so many fun ways to crash!
*
* the while(true){} is inserted so compiler doesn't complain about [[noreturn]] function returning
*/

#define GLOBED_CRASH_SIGSEGV *((volatile int*)nullptr) = 0x69; while(true) {}

#ifdef GLOBED_UNIX
# include <signal.h>
# define GLOBED_CRASH_SIGTRAP raise(SIGTRAP); while(true) {}
#else
# include <intrin.h>
# define GLOBED_CRASH_SIGTRAP __debugbreak(); while(true) {}
#endif


#if defined(__clang__) || defined(__GNUC__)
# define GLOBED_CRASH_SIGILL __builtin_trap(); while(true) {}
#elif defined(GLOBED_X86) && !defined(GLOBED_X64) // msvc x86
# define GLOBED_CRASH_SIGILL __asm ud2; while(true) {}
#else // msvc x64 has no inline asm and no __builtin_trap() so we cant do much
# define GLOBED_CRASH_SIGILL GLOBED_CRASH_SIGTRAP
#endif


#define GLOBED_SUICIDE GLOBED_CRASH_SIGTRAP