#pragma once
#include <iostream>

#define GLOBED_ASSERT_LOG(content) (std::cerr << content << std::endl);
#define GLOBED_ROOT_NO_GEODE
#define GLOBED_TESTING
#define GLOBED_IGNORE_CONFIG_HPP
#define GLOBED_FORCE_CONSTEVAL 0
#include <defs.hpp>