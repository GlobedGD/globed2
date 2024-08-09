#pragma once

#include <string_view>

#include <defs/minimal_geode.hpp>
#include <util/data.hpp>
#include "adler32.hpp"

#define CRYPTO_REQUIRE(condition, message) GLOBED_REQUIRE(condition, "crypto error: " message)
#define CRYPTO_REQUIRE_SAFE(condition, message) GLOBED_REQUIRE_SAFE(condition, "crypto error: " message)
#define CRYPTO_ERR_CHECK(result, message) CRYPTO_REQUIRE(result == 0, message)
#define CRYPTO_ERR_CHECK_SAFE(result, message) CRYPTO_REQUIRE_SAFE(result == 0, message)

namespace util::crypto {
    // generate `size` bytes of cryptographically secure random data
    data::bytevector secureRandom(size_t size);
    // generate `size` bytes of cryptographically secure random data into the given buffer
    void secureRandom(data::byte* dest, size_t size);

    // generate a hash from this string and return it together with the salt prepended
    data::bytevector pwHash(const std::string_view input);
    // generate a hash from this bytevector and return it together with the salt prepended
    data::bytevector pwHash(const data::bytevector& input);
    // generate a hash from this buffer and return it together with the salt prepended
    data::bytevector pwHash(const data::byte* input, size_t len);

    // generate a simple, consistent hash from this string
    data::bytevector simpleHash(const std::string_view input);
    // generate a simple, consistent hash from this bytevector
    data::bytevector simpleHash(const data::bytevector& input);
    // generate a simple, consistent hash from this buffer
    data::bytevector simpleHash(const data::byte* input, size_t size);

    // compares two strings in constant time to prevent timing attacks
    bool stringsEqual(const std::string_view s1, const std::string_view s2);

    enum class Base64Variant {
        STANDARD,
        STANDARD_NO_PAD,
        URLSAFE,
        URLSAFE_NO_PAD,
    };

    // encodes the given buffer into a base64 string
    std::string base64Encode(const data::byte* source, size_t size, Base64Variant variant = Base64Variant::STANDARD);
    // encodes the given bytevector into a base64 string
    std::string base64Encode(const data::bytevector& source, Base64Variant variant = Base64Variant::STANDARD);
    // encodes the given string into a base64 string
    std::string base64Encode(const std::string_view source, Base64Variant variant = Base64Variant::STANDARD);

    // decodes the given base64 buffer into a bytevector
    Result<data::bytevector> base64Decode(const data::byte* source, size_t size, Base64Variant variant = Base64Variant::STANDARD);
    // decodes the given base64 string into a bytevector
    Result<data::bytevector> base64Decode(const std::string_view source, Base64Variant variant = Base64Variant::STANDARD);
    // decodes the given base64 bytevector into a bytevector
    Result<data::bytevector> base64Decode(const data::bytevector& source, Base64Variant variant = Base64Variant::STANDARD);

    // encodes the given buffer into a hex string
    std::string hexEncode(const data::byte* source, size_t size);
    // encodes the given bytevector into a hex string
    std::string hexEncode(const data::bytevector& source);
    // encodes the given string into a hex string
    std::string hexEncode(const std::string_view source);

    // decodes the given hex buffer into a bytevector
    Result<data::bytevector> hexDecode(const data::byte* source, size_t size);
    // decodes the given hex string into a bytevector
    Result<data::bytevector> hexDecode(const std::string_view source);
    // decodes the given hex bytevector into a bytevector
    Result<data::bytevector> hexDecode(const data::bytevector& source);

    // convert a `Base64Variant` enum to an int for libsodium API
    int base64VariantToInt(Base64Variant variant);
}
