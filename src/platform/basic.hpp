#include <Geode/platform/cplatform.h>

#ifdef GEODE_IS_ARM_MAC
# define GLOBED_ARM
#elif defined(GEODE_IS_ANDROID)
# define GLOBED_ARM
#else
# define GLOBED_X86
#endif

#if UINTPTR_MAX > 0xffffffff
# ifdef GLOBED_ARM
#  define GLOBED_ARM64
# else
#  define GLOBED_X64
#  define GLOBED_X86_64
# endif
#endif