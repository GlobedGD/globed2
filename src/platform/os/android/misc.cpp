#include <defs/minimal_geode.hpp>
#include <util/misc.hpp>
#include <util/crypto.hpp>

#include <android/configuration.h>

using util::misc::UniqueIdent;

Result<UniqueIdent> util::misc::fingerprintImpl() {
    char value[1024] = {0};

    constexpr static auto tryKeys = {
        "ro.serialno",
        "ro.boot.serialno",
        "ro.build.fingerprint",
        "ro.product.build.fingerprint",
        "ro.system.build.id"
    };

    for (auto key : tryKeys) {
        if (__system_property_get(key, value) >= 1) break;
    }

    if (value[0] == '\0') {
        return Err("getprop failed");
    }

    log::debug("Fingerprint: {}", value);

    auto hashed = util::crypto::simpleHash(fmt::format("{}-{}", value, Mod::get()->getID()));
    std::array<uint8_t, 32> arr;
    std::copy_n(hashed.data(), 32, arr.begin());

    return Ok(UniqueIdent(arr));
}
