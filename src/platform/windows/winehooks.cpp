#include <globed/core/SettingsManager.hpp>
#include <globed/config.hpp>

using namespace geode::prelude;

// _Throw_Cpp_error reimpl

static constexpr const char* msgs[] = {
    // error messages
    "device or resource busy",
    "invalid argument",
    "no such process",
    "not enough memory",
    "operation not permitted",
    "resource deadlock would occur",
    "resource unavailable try again",
};

using errc = std::errc;

static constexpr errc codes[] = {
    // system_error codes
    errc::device_or_resource_busy,
    errc::invalid_argument,
    errc::no_such_process,
    errc::not_enough_memory,
    errc::operation_not_permitted,
    errc::resource_deadlock_would_occur,
    errc::resource_unavailable_try_again,
};

[[noreturn]] void GLOBED_DLL __cdecl _throw_cpp_error_hook(int code) {
    throw std::system_error((int) codes[code], std::generic_category(), msgs[code]);
}

static void fixThrowCppErrorStub() {
    auto address = reinterpret_cast<void*>(&std::_Throw_Cpp_error);

    // movabs rax, <hook>
    std::vector<uint8_t> patchBytes = {
        0x48, 0xb8
    };

    for (auto byte : geode::toBytes(&_throw_cpp_error_hook)) {
        patchBytes.push_back(byte);
    }

    // jmp rax
    patchBytes.push_back(0xff);
    patchBytes.push_back(0xe0);

    if (auto res = Mod::get()->patch(address, patchBytes)) {} else {
        log::warn("_Throw_Cpp_error hook failed: {}", res.unwrapErr());
    }
}

static bool isWine() {
    auto dll = LoadLibraryW(L"ntdll.dll");
    return GetProcAddress(dll, "wine_get_version") != nullptr;
}

namespace globed {
    void platformSetup() {
        if (isWine()) {
            fixThrowCppErrorStub();
        }
    }
}

