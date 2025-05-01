#include <defs/assert.hpp>

#include <util/debug.hpp>

using namespace geode::prelude;

#ifdef GLOBED_ENABLE_STACKTRACE

# include <cpptrace/cpptrace.hpp>

static void printStacktrace(const cpptrace::stacktrace& trace) {
    log::debug("\n{}", trace.to_string(true));
}

#endif

[[noreturn]] void globed::_condFail(const std::source_location& loc, std::string_view message) {
    throw std::runtime_error(_condFailSafe(loc, message));
}

std::string globed::_condFailSafe(const std::source_location& loc, std::string_view message) {
    log::debug("Condition failed at {} ({}, line {})", loc.function_name(), loc.file_name(), loc.line());

#ifdef GLOBED_ENABLE_STACKTRACE
    printStacktrace(cpptrace::generate_trace());
#endif

    return std::string(message);
}

[[noreturn]] void globed::_condFailFatal(const std::source_location& loc, std::string_view message) {
    log::error("Condition fatally failed: {}", message);
    log::error("At {} ({}, line {})", loc.function_name(), loc.file_name(), loc.line());

#ifdef GLOBED_ENABLE_STACKTRACE
    printStacktrace(cpptrace::generate_trace());
#endif

    util::debug::suicide();
}
