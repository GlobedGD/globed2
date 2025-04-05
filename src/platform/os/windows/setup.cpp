#include <defs/geode.hpp>
#include <util/debug.hpp>
#include <managers/settings.hpp>

using namespace geode::prelude;

// TODO: all of this is probably obsolete
// I am only leaving it here in the case that this somehow becomes an issue in the future again

// Ok so this is really cursed but let me explain
// somewhere sometime recently microsoft stl brokey the internal mutex or cv structure or whatever
// and wine kinda doesnt wanna work with it (even with most recent redistributable package)
// tbh i dont entirely understand this either but hey it is what it is

// this applies to latest sdk. the --sdk-version argument in xwin does nothing here.
//
// if you want to compile globed on linux without broken conditional variables, the steps are rather simple:
// 1. look up https://aka.ms/vs/17/release/channel on the web archive
// 2. get the entry from july, inside the file you should see version as "17.10", not "17.11" or later
// 3. open .xwin-cache/dl/manifest_17.json and replace the contents
// 4. run xwin install command again and it should work now!

// ALTERNATIVELY

// if you are very pissed, just change the '#if 0' to '#if 1'.
// it is a temp fix and it WILL break things. but it does not crash on startup anymore! (most of the time)

#if 0

static inline bool _Primitive_wait_for(const _Cnd_t cond, const _Mtx_t mtx, unsigned int timeout) noexcept {
    const auto pcv  = reinterpret_cast<PCONDITION_VARIABLE>((uintptr_t)cond + 8);
    const auto psrw = reinterpret_cast<PSRWLOCK>(&mtx->_Critical_section._Unused); // changed from &mtx->_Critical_section._M_srw_lock
    return SleepConditionVariableSRW(pcv, psrw, timeout, 0) != 0;
}

static _Thrd_result __stdcall _Cnd_timedwait_for_reimpl(_Cnd_t cond, _Mtx_t mtx, unsigned int target_ms) noexcept {
    _Thrd_result res            = _Thrd_result::_Success;
    unsigned long long start_ms = 0;

    start_ms = GetTickCount64();

    // TRANSITION: replace with _Mtx_clear_owner(mtx);
    mtx->_Thread_id = -1;
    --mtx->_Count;

    if (!_Primitive_wait_for(cond, mtx, target_ms)) { // report timeout
        if (GetTickCount64() - start_ms >= target_ms) {
            res = _Thrd_result::_Timedout;
        }
    }
    // TRANSITION: replace with _Mtx_reset_owner(mtx);
    mtx->_Thread_id = static_cast<long>(GetCurrentThreadId());
    ++mtx->_Count;

    return res;
}

static void fixCVAbiBreak() {
    if (GlobedSettings::get().launchArgs().crtFix) {
        (void) Mod::get()->hook(
            reinterpret_cast<void*>(addresser::getNonVirtual(&_Cnd_timedwait_for_unchecked)),
            _Cnd_timedwait_for_reimpl,
            "_Cnd_timedwait_for",
            tulip::hook::TulipConvention::Default
        );
    }
}

#else

static void fixCVAbiBreak() {}

#endif

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

namespace globed {
    void platformSetup() {
        if (util::debug::isWine()) {
            fixCVAbiBreak();
            fixThrowCppErrorStub();
        }
    }
}

