#pragma once
#include <iostream>

#define GLOBED_ASSERT_LOG(content) (std::cerr << content << std::endl);
#define GLOBED_ROOT_NO_GEODE
#include <config.hpp>
#undef GLOBED_USE_SOURCE_LOCATION
#define GLOBED_USE_SOURCE_LOCATION 0
#include <defs.hpp>