#pragma once
#include <qunet/util/assert.hpp>

#define GLOBED_ASSERT(c) QN_ASSERT(c)
#ifdef GLOBED_DEBUG
# define GLOBED_DEBUG_ASSERT(c) GLOBED_ASSERT(c)
#else
# define GLOBED_DEBUG_ASSERT(c) ((void)0)
#endif
