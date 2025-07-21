#pragma once
#include <Geode/platform/cplatform.h>

// Dllexport macro (note: globed2_EXPORTS is not defined in cmakelists, it's an internal cmake macro)

#ifdef _WIN32
# ifdef globed2_EXPORTS
#  define GLOBED_DLL __declspec(dllexport)
# else
#  define GLOBED_DLL __declspec(dllimport)
# endif
#else
# ifdef globed2_EXPORTS
#  define GLOBED_API __attribute__((visibility("default")))
# else
#  define GLOBED_DLL
# endif
#endif

// Various attribute macros

#if defined(GEODE_IS_WINDOWS)
# define GLOBED_NOVTABLE __declspec(novtable)
# define GLOBED_NOINLINE __declspec(noinline)
# define GLOBED_ALWAYS_INLINE __forceinline
#else
# define GLOBED_NOVTABLE
# define GLOBED_NOINLINE __attribute__((noinline))
# define GLOBED_ALWAYS_INLINE __attribute__((always_inline)) inline
#endif
