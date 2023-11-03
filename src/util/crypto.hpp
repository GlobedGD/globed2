#pragma once
#include <defs.hpp>
#include "data.hpp"

#define CRYPTO_ASSERT(condition, message) GLOBED_ASSERT(condition, "crypto error: " message)
#define CRYPTO_ERR_CHECK(result, message) CRYPTO_ASSERT(result == 0, message)

// sodium_init returns 0 on success, 1 if already initialized, -1 on fail
#define CRYPTO_SODIUM_INIT CRYPTO_ASSERT(sodium_init() != -1, "sodium_init failed")

namespace util::crypto {
    /*
    * pwHash is better for sensitive data, as it has a salt
    * simpleHash is better for less sensitive data, as unlike pwHash, the output will always be the same given the same input.
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
};
