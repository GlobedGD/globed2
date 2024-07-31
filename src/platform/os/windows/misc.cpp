#define _WIN32_DCOM

#include <defs/minimal_geode.hpp>
#include <util/misc.hpp>
#include <util/crypto.hpp>
#include <comdef.h>
#include <Wbemidl.h>

using util::misc::UniqueIdent;

static bool isWine() {
    auto dll = LoadLibraryW(L"ntdll.dll");
    return GetProcAddress(dll, "wine_get_version") != nullptr;
}

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
    auto _u = util::misc::scopeDestructor([&] {
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

static Result<std::string> getWMIUniqueIdent() {
    HRESULT hres;
    hres = CoInitializeEx(0, COINIT_MULTITHREADED);

    if (FAILED(hres)) {
        return Err(fmt::format("Failed to initialize COM: error {}", hres));
    }

    // uninit com before returning
    auto _u = util::misc::scopeDestructor([] {
        CoUninitialize();
    });

    hres = CoInitializeSecurity(
        NULL,
        -1,
        NULL,
        NULL,
        RPC_C_AUTHN_LEVEL_DEFAULT,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL,
        EOAC_NONE,
        NULL
    );

    if (FAILED(hres)) {
        return Err(fmt::format("Failed to initialize security: error {}", hres));
    }

    IWbemLocator* locator = nullptr;
    hres = CoCreateInstance(
        CLSID_WbemLocator,
        0,
        CLSCTX_INPROC_SERVER,
        IID_IWbemLocator,
        (LPVOID*) &locator
    );

    if (FAILED(hres)) {
        return Err(fmt::format("Failed to create IWbemLocator: error {}", hres));
    }

    // release locator
    auto _u2 = util::misc::scopeDestructor([&] {
        locator->Release();
    });

    IWbemServices* services = nullptr;
    hres = locator->ConnectServer(
        _bstr_t(L"ROOT\\CIMV2"),
        NULL,
        NULL,
        0,
        NULL,
        0,
        NULL,
        &services
    );

    if (FAILED(hres)) {
        return Err(fmt::format("Could not connect to the WMI server: error {}", hres));
    }

    // release services
    auto _u3 = util::misc::scopeDestructor([&] {
        services->Release();
    });

    hres = CoSetProxyBlanket(
        services,
        RPC_C_AUTHN_WINNT,
        RPC_C_AUTHZ_NONE,
        NULL,
        RPC_C_AUTHN_LEVEL_CALL,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL,
        EOAC_NONE
    );

    if (FAILED(hres)) {
        return Err(fmt::format("Could not set proxy blanket: error {}", hres));
    }

    IEnumWbemClassObject* enumerator = nullptr;
    hres = services->ExecQuery(
        bstr_t("WQL"),
        bstr_t("SELECT SerialNumber FROM Win32_PhysicalMedia"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        &enumerator
    );

    if (FAILED(hres)) {
        return Err(fmt::format("Could not execute query: error {}", hres));
    }

    IWbemClassObject* clsobj = nullptr;
    ULONG uReturn = 0;

    std::string diskSerial;

    while (enumerator) {
        HRESULT hr = enumerator->Next(WBEM_INFINITE, 1, &clsobj, &uReturn);
        if (uReturn == 0) {
            break;
        }

        VARIANT vtProp;
        VariantInit(&vtProp);

        hr = clsobj->Get(L"SerialNumber", 0, &vtProp, 0, 0);
        if (diskSerial.empty()) diskSerial = geode::utils::string::wideToUtf8(vtProp.bstrVal);

        VariantClear(&vtProp);

        clsobj->Release();
    }

    enumerator->Release();

    if (diskSerial.empty()) {
        return Err("no disks");
    }

    return Ok(diskSerial);
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

Result<UniqueIdent> util::misc::fingerprintImpl() {
    std::string cpuName = getCPUName();
    std::string guid = getMachineGuid();
    std::string finalString;

    auto res = getWMIUniqueIdent();

    if (res) {
        finalString = fmt::format("{}-{}-{}", res.unwrap(), cpuName, Mod::get()->getID());

        if (isWine()) {
            finalString += "-" + guid; // bc wmi ident isnt unique enough
        }
    } else {
        log::warn("Failed to query WMI: {}", res.unwrapErr());
        finalString = fmt::format("{}-{}-{}", cpuName, Mod::get()->getID(), guid);
    }

    auto hashed = util::crypto::simpleHash(finalString);
    std::array<uint8_t, 32> arr;
    std::copy_n(hashed.data(), 32, arr.begin());

    return Ok(UniqueIdent(arr));
}
