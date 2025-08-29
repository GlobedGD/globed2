#include <core/net/NetworkManagerImpl.hpp>
#include <qunet/util/hash.hpp>

using namespace geode::prelude;

namespace globed {

template <typename F>
struct ScopeGuard {
    ScopeGuard(F&& f) : f(std::forward<F>(f)) {}

    ~ScopeGuard() {
        f();
    }

private:
    std::decay_t<F> f;
};

static std::string getMachineGuid() {
    HKEY hkey;
    if (ERROR_SUCCESS != RegOpenKeyExA(
        HKEY_LOCAL_MACHINE,
        "SOFTWARE\\Microsoft\\Cryptography",
        0,
        KEY_READ | KEY_WOW64_64KEY,
        &hkey)
    ) {
        log::warn("Failed to open registry key for machine GUID");
        return "";
    }

    // free the key
    ScopeGuard _g([&] {
        RegCloseKey(hkey);
    });

    char data[256];
    DWORD dataSize = sizeof(data);

    if (ERROR_SUCCESS != RegQueryValueExA(
        hkey,
        "MachineGuid",
        NULL,
        NULL,
        (LPBYTE)data,
        &dataSize
    )) {
        log::warn("Failed to query registry value for machine GUID");
        return "";
    }

    return std::string(data, data + dataSize - 1);
}

// thanks to Prevter - https://github.com/Prevter/BetterCrashlogs/blob/bf6ae0056cdfed6fc9ec3612387c1751d6807707/src/utils/hwinfo.cpp#L62
static std::string getCPUName() {
    std::array<int, 4> integerBuffer = {};
    constexpr size_t sizeofIntegerBuffer = sizeof(int) * integerBuffer.size();

    std::array<char, 64> charBuffer = {};
    constexpr std::array<int, 4> functionIds = {
        static_cast<int>(0x8000'0002),
        static_cast<int>(0x8000'0003),
        static_cast<int>(0x8000'0004)
    };

    std::string cpu;

    for (auto& id : functionIds) {
        __cpuid(integerBuffer.data(), id);
        std::memcpy(charBuffer.data(), integerBuffer.data(), sizeofIntegerBuffer);
        cpu += std::string(charBuffer.data());
    }

    cpu.erase(std::find_if(cpu.rbegin(), cpu.rend(), [](int ch) {
        return !std::isspace(ch);
    }).base(), cpu.end());

    return cpu;
}

Result<std::array<uint8_t, 32>> NetworkManagerImpl::computeUidentInner() {
    std::string cpuName = getCPUName();
    std::string guid = getMachineGuid();
    std::string finalString = fmt::format("{}-{}-{}", cpuName, Mod::get()->getID(), guid);

    auto hashed = qn::blake3Hash(finalString);
    std::array<uint8_t, 32> arr;
    std::copy_n(hashed.data, 32, arr.begin());

    return Ok(arr);
}

}