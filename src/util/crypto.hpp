#pragma once
#include "data.hpp"

#include <defs.hpp>
#include <sodium.h>

#define CRYPTO_ASSERT(condition, message) GLOBED_ASSERT(condition, "crypto error: " message)
#define CRYPTO_ERR_CHECK(result, message) CRYPTO_ASSERT(result == 0, message)

// sodium_init returns 0 on success, 1 if already initialized, -1 on fail
#define CRYPTO_SODIUM_INIT CRYPTO_ASSERT(sodium_init() != -1, "sodium_init failed")

namespace util::crypto {
    // generate `size` bytes of securely generated random data
    util::data::bytevector secureRandom(size_t size);
    // generate `size` bytes of securely generated random data into the given buffer
    void secureRandom(util::data::byte* dest, size_t size);

    /*
    * pwHash is better for sensitive data, as it has a salt
    * simpleHash is deterministic and uses Blake2b-256.
    */

    // generate a hash from this string and return it together with the salt prepended
    util::data::bytevector pwHash(const std::string& input);
    // generate a hash from this bytevector and return it together with the salt prepended
    util::data::bytevector pwHash(const util::data::bytevector& input);
    // generate a hash from this byte array and return it together with the salt prepended
    util::data::bytevector pwHash(const util::data::byte* input, size_t len);

    // generate a simple, consistent hash from this string
    util::data::bytevector simpleHash(const std::string& input);
    // generate a simple, consistent hash from this bytevector
    util::data::bytevector simpleHash(const util::data::bytevector& input);
    // generate a simple, consistent hash from this byte array
    util::data::bytevector simpleHash(const util::data::byte* input, size_t size);
    
    // generate a 6-digit TOTP code given the key (rfc 6238 compliant i think)
    std::string simpleTOTP(const util::data::byte* key, size_t keySize);
    // generate a 6-digit TOTP code given the key (rfc 6238 compliant i think)
    std::string simpleTOTP(const util::data::bytevector& key);

    // generate a 6-digit TOTP code given the key and the time period. use simpleTOTP instead to use current time.
    std::string simpleTOTPForPeriod(const util::data::byte* key, size_t keySize, uint64_t period);

    // verify the TOTP code given a key and an optional arg skew.
    bool simpleTOTPVerify(const std::string& code, const util::data::byte* key, size_t keySize, size_t skew = 1);
    // verify the TOTP code given a key and an optional arg skew.
    bool simpleTOTPVerify(const std::string& code, const util::data::bytevector& key, size_t skew = 1);

    // compares two strings in constant time to prevent timing attacks
    bool stringsEqual(const std::string& s1, const std::string& s2);

    enum class Base64Variant {
        ORIGINAL = sodium_base64_VARIANT_ORIGINAL,
        ORIGINAL_NO_PAD = sodium_base64_VARIANT_ORIGINAL_NO_PADDING,
        URLSAFE = sodium_base64_VARIANT_URLSAFE,
        URLSAFE_NO_PAD = sodium_base64_VARIANT_URLSAFE_NO_PADDING,
    };

    // encodes the given byte array into a base64 string
    std::string base64Encode(const util::data::byte* source, size_t size, Base64Variant variant = Base64Variant::ORIGINAL);
    // encodes the given bytevector into a base64 string
    std::string base64Encode(const util::data::bytevector& source, Base64Variant variant = Base64Variant::ORIGINAL);
    // encodes the given string into a base64 string
    std::string base64Encode(const std::string& source, Base64Variant variant = Base64Variant::ORIGINAL);

    // decodes the given base64 byte array into a bytevector 
    util::data::bytevector base64Decode(const util::data::byte* source, size_t size, Base64Variant variant = Base64Variant::ORIGINAL);
    // decodes the given base64 string into a bytevector
    util::data::bytevector base64Decode(const std::string& source, Base64Variant variant = Base64Variant::ORIGINAL);
    // decodes the given base64 bytevector into a bytevector 
    util::data::bytevector base64Decode(const util::data::bytevector& source, Base64Variant variant = Base64Variant::ORIGINAL);
    
    // encodes the given byte array into a hex string
    std::string hexEncode(const util::data::byte* source, size_t size);
    // encodes the given bytevector into a hex string
    std::string hexEncode(const util::data::bytevector& source);
    // encodes the given string into a hex string
    std::string hexEncode(const std::string& source);

    // decodes the given hex byte array into a bytevector
    util::data::bytevector hexDecode(const util::data::byte* source, size_t size);
    // decodes the given hex string into a bytevector
    util::data::bytevector hexDecode(const std::string& source);
    // decodes the given hex bytevector into a bytevector
    util::data::bytevector hexDecode(const util::data::bytevector& source);
};
