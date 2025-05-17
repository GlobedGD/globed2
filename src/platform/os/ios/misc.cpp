#include <defs/minimal_geode.hpp>

#include <util/misc.hpp>
#include <util/crypto.hpp>

using util::misc::UniqueIdent;

std::string getVendorId(); // objc

static std::string getUuid() {
    return getVendorId();
}

Result<UniqueIdent> util::misc::fingerprintImpl() {
    auto uuid = getUuid();
    if (uuid.empty()) {
        return Err("Failed to get UUID");
    }

    auto hashed = util::crypto::simpleHash(fmt::format("{}-{}", uuid, Mod::get()->getID()));
    std::array<uint8_t, 32> arr;
    std::copy_n(hashed.data(), 32, arr.begin());

    return Ok(UniqueIdent(arr));
}
