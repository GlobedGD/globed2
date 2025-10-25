#include <globed/util/assert.hpp>
#include <fmt/format.h>
#include <Geode/utils/terminate.hpp>

void globed::_assertionFail(std::string_view what, std::string_view file, int line) {
    geode::utils::terminate(fmt::format("Assertion failed ({}) at {}:{}", what, file, line));
}
