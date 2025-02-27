#include <defs/assert.hpp>

#include <util/debug.hpp>

using namespace geode::prelude;

#ifdef GLOBED_DEBUG

#include <cpptrace/cpptrace.hpp>

struct Module {
    std::string name;
    uintptr_t base;
};

# ifdef GEODE_IS_WINDOWS

// this code is very borrowed from geode
static HMODULE handleFromAddress(void const* addr) {
    HMODULE module = nullptr;
    GetModuleHandleEx(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCTSTR)addr, &module
    );
    return module;
}

static std::string getModuleName(HMODULE module, bool fullPath = true) {
    char buffer[MAX_PATH];
    if (!GetModuleFileNameA(module, buffer, MAX_PATH)) {
        return "<Unknown>";
    }
    if (fullPath) {
        return buffer;
    }
    return std::filesystem::path(buffer).filename().string();
}

template <typename T>
static std::optional<Module> moduleFromAddress(T addr_) {
    auto addr = reinterpret_cast<const void*>(addr_);
    auto handle = handleFromAddress(addr);
    if (!handle) {
        return std::nullopt;
    }

    Module mod;

    mod.base = reinterpret_cast<uintptr_t>(handle);
    mod.name = getModuleName(handle);

    return mod;
}

# else
template <typename T>
static std::optional<Module> moduleFromAddress(T addr_) {
    return std::nullopt;
}
#endif

static void printStacktrace(const cpptrace::stacktrace& trace) {
    log::debug("\n{}", trace.to_string(true));
}

[[noreturn]] void globed::_condFail(const std::source_location& loc, std::string_view message) {
    throw std::runtime_error(_condFailSafe(loc, message));
}

std::string globed::_condFailSafe(const std::source_location& loc, std::string_view message) {
    log::debug("Condition failed at {} ({}, line {})", loc.function_name(), loc.file_name(), loc.line());

#ifdef GLOBED_DEBUG
    printStacktrace(cpptrace::generate_trace());
#endif

    return std::string(message);
}

[[noreturn]] void globed::_condFailFatal(const std::source_location& loc, std::string_view message) {
    log::error("Condition fatally failed: {}", message);
    log::error("At {} ({}, line {})", loc.function_name(), loc.file_name(), loc.line());

#ifdef GLOBED_DEBUG
    printStacktrace(cpptrace::generate_trace());
#endif

    util::debug::suicide();
}

#endif
