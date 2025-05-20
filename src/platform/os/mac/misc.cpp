#include <defs/minimal_geode.hpp>

#include <util/misc.hpp>
#include <util/crypto.hpp>

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>

using util::misc::UniqueIdent;

std::string getVendorId(); // objc

static std::string getUuid() {
    io_registry_entry_t ioRegistryRoot = IORegistryEntryFromPath(kIOMasterPortDefault, "IOService:/");
    CFStringRef uuidCf = (CFStringRef)IORegistryEntryCreateCFProperty(ioRegistryRoot, CFSTR(kIOPlatformUUIDKey), kCFAllocatorDefault, 0);
    IOObjectRelease(ioRegistryRoot);

    if (!uuidCf) {
        return "";
    }

    char uuid[256];
    if (CFStringGetCString(uuidCf, uuid, sizeof(uuid), kCFStringEncodingUTF8)) {
        CFRelease(uuidCf);
        return std::string(uuid);
    }

    CFRelease(uuidCf);
    return "";
}

Result<UniqueIdent> util::misc::fingerprintImpl() {
    auto uuid = getUuid();
    if (uuid.empty()) {
        uuid = getVendorId();
    }

    if (uuid.empty()) {
        return Err("Failed to get UUID");
    }

    auto hashed = util::crypto::simpleHash(fmt::format("{}-{}", uuid, Mod::get()->getID()));
    std::array<uint8_t, 32> arr;
    std::copy_n(hashed.data(), 32, arr.begin());

    return Ok(UniqueIdent(arr));
}
