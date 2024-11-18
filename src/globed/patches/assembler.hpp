#pragma once
#include <asp/simd/base.hpp>

#ifdef ASP_IS_X64
# include "assembler-x64.hpp"
#elif defined (ASP_IS_ARM64)
# include "assembler-arm64.hpp"
#elif defined (ASP_IS_ARM)
# include "assembler-armv7.hpp"
#else
# error "Unsupported architecture"
#endif
