#include "crypto.hpp"

#include <sodium.h>

#include <defs/assert.hpp>
#include <defs/minimal_geode.hpp>
#include <cstring>
#include <cmath>
#include <iomanip>
#include <sstream>

using namespace util::data;

namespace util::crypto {

bytevector secureRandom(size_t size) {
    bytevector out(size);
    secureRandom(out.data(), size);
    return out;
}

void secureRandom(byte* dest, size_t size) {
    randombytes_buf(dest, size);
}

bytevector pwHash(const std::string_view input) {
    return pwHash(reinterpret_cast<const byte*>(input.data()), input.size());
}

bytevector pwHash(const bytevector& input) {
    return pwHash(reinterpret_cast<const byte*>(input.data()), input.size());
}

bytevector pwHash(const byte* input, size_t len) {
    bytevector out(crypto_pwhash_SALTBYTES + crypto_secretbox_KEYBYTES);
    byte* salt = out.data();
    byte* key = out.data() + crypto_pwhash_SALTBYTES;
    randombytes_buf(salt, crypto_pwhash_SALTBYTES);

    CRYPTO_ERR_CHECK(crypto_pwhash(
        key,
        crypto_secretbox_KEYBYTES,
        reinterpret_cast<const char*>(input),
        len,
        salt,
        crypto_pwhash_OPSLIMIT_INTERACTIVE,
        crypto_pwhash_MEMLIMIT_INTERACTIVE,
        crypto_pwhash_ALG_DEFAULT
    ), "crypto_pwhash failed")

    return out;
}

bytevector simpleHash(const std::string_view input) {
    return simpleHash(reinterpret_cast<const byte*>(input.data()), input.size());
}

bytevector simpleHash(const bytevector& input) {
    return simpleHash(reinterpret_cast<const byte*>(input.data()), input.size());
}

bytevector simpleHash(const byte* input, size_t size) {
    bytevector out(crypto_generichash_BYTES);
    CRYPTO_ERR_CHECK(crypto_generichash(
        out.data(),
        crypto_generichash_BYTES,
        input, size,
        nullptr, 0
    ), "crypto_generichash failed")

    return out;
}


std::string simpleTOTP(const byte* key, size_t keySize) {
    return simpleTOTPForPeriod(key, keySize, std::time(nullptr) / 30);
}

std::string simpleTOTP(const bytevector& key) {
    return simpleTOTP(key.data(), key.size());
}

std::string simpleTOTPForPeriod(const byte *key, size_t keySize, uint64_t period) {
    CRYPTO_REQUIRE(keySize == crypto_auth_hmacsha256_KEYBYTES, "invalid key size passed to simpleTOTPForPeriod")

    if constexpr (GLOBED_LITTLE_ENDIAN) {
        period = byteswap(period);
    }

    byte hmacResult[crypto_auth_hmacsha256_BYTES];
    crypto_auth_hmacsha256(hmacResult, reinterpret_cast<const byte*>(&period), sizeof(period), key);

    // Calculate the offset
    size_t offset = hmacResult[crypto_auth_hmacsha256_BYTES - 1] & 0x0F;

    // Calculate the 4-byte dynamic binary code (what the fuck is this?)
    uint32_t binaryCode =
        (hmacResult[offset] & 0x7f) << 24 |
        (hmacResult[offset + 1]) << 16 |
        (hmacResult[offset + 2]) << 8 |
        (hmacResult[offset + 3]);

    // Calculate the 6-digit OTP
    uint32_t otp = binaryCode % 1000000;

    // Convert the OTP to a string with leading zeros
    std::ostringstream otpStream;
    otpStream << std::setw(6) << std::setfill('0') << otp;

    return otpStream.str();
}

bool simpleTOTPVerify(const std::string_view code, const byte* key, size_t keySize, size_t skew) {
    uint64_t curPeriod = std::time(nullptr) / 30;
    auto curTotp = simpleTOTPForPeriod(key, keySize, curPeriod);
    if (stringsEqual(code, curTotp)) {
        return true;
    }

    // skew indicates the window where the code is still valid.
    // so skew = 1 means current period, 1 before and 1 after, total 90 seconds.
    // skew = 0 means only current period.
    // using values other than 1 is really not recommended.
    for (size_t i = 0; i < skew; i++) {
        auto beforeTotp = simpleTOTPForPeriod(key, keySize, curPeriod - i);
        if (stringsEqual(code, beforeTotp)) {
            return true;
        }

        auto afterTotp = simpleTOTPForPeriod(key, keySize, curPeriod + i);
        if (stringsEqual(code, afterTotp)) {
            return true;
        }
    }

    return false;
}

bool simpleTOTPVerify(const std::string_view code, const bytevector& key, size_t skew) {
    return simpleTOTPVerify(code, key.data(), key.size(), skew);
}

bool stringsEqual(const std::string_view s1, const std::string_view s2) {
    if (s1.size() != s2.size()) {
        return false;
    }

    char result = 0;
    for (size_t i = 0; i < s1.size(); i++) {
        result |= (s1[i] ^ s2[i]);
    }

    return (result == 0);
}

std::string base64Encode(const byte* source, size_t size, Base64Variant variant) {
    size_t length = sodium_base64_ENCODED_LEN(size, (int)variant);

    std::string ret;
    ret.resize(length);

    sodium_bin2base64(ret.data(), length, source, size, base64VariantToInt(variant));

    ret.resize(length - 1); // get rid of the trailing null byte

    return ret;
}

std::string base64Encode(const bytevector& source, Base64Variant variant) {
    return base64Encode(source.data(), source.size(), variant);
}

std::string base64Encode(const std::string_view source, Base64Variant variant) {
    return base64Encode(reinterpret_cast<const byte*>(source.data()), source.size(), variant);
}

bytevector base64Decode(const byte* source, size_t size, Base64Variant variant) {
    auto outMaxLen = size / 4 * 3;
    bytevector out(outMaxLen);

    size_t outRealLen;
    // crazy
    CRYPTO_ERR_CHECK(sodium_base642bin(
        out.data(), outMaxLen,
        reinterpret_cast<const char*>(source), size,
        nullptr, &outRealLen, nullptr, base64VariantToInt(variant)
    ), "invalid base64 string")

    out.resize(outRealLen); // necessary

    return out;
}

bytevector base64Decode(const std::string_view source, Base64Variant variant) {
    return base64Decode(reinterpret_cast<const byte*>(source.data()), source.size(), variant);
}

bytevector base64Decode(const bytevector& source, Base64Variant variant) {
    return base64Decode(source.data(), source.size(), variant);
}

std::string hexEncode(const byte* source, size_t size) {
    auto outLen = size * 2 + 1;

    std::string ret;
    ret.resize(outLen);

    sodium_bin2hex(ret.data(), outLen, source, size);

    ret.resize(outLen - 1); // get rid of the trailing null byte

    return ret;
}

std::string hexEncode(const bytevector& source) {
    return hexEncode(source.data(), source.size());
}

std::string hexEncode(const std::string_view source) {
    return hexEncode(reinterpret_cast<const byte*>(source.data()), source.size());
}

bytevector hexDecode(const byte* source, size_t size) {
    size_t outLen = size / 2;
    bytevector out(outLen);

    size_t realOutLen;

    CRYPTO_ERR_CHECK(sodium_hex2bin(
        out.data(), outLen,
        reinterpret_cast<const char*>(source), size,
        nullptr, &realOutLen, nullptr
    ), "invalid hex string")

    out.resize(realOutLen);

    return out;
}

bytevector hexDecode(const std::string_view source) {
    return hexDecode(reinterpret_cast<const byte*>(source.data()), source.size());
}

bytevector hexDecode(const bytevector& source) {
    return hexDecode(source.data(), source.size());
}

int base64VariantToInt(Base64Variant variant) {
    switch (variant) {
    case Base64Variant::STANDARD: return sodium_base64_VARIANT_ORIGINAL;
    case Base64Variant::STANDARD_NO_PAD: return sodium_base64_VARIANT_ORIGINAL_NO_PADDING;
    case Base64Variant::URLSAFE: return sodium_base64_VARIANT_URLSAFE;
    case Base64Variant::URLSAFE_NO_PAD: return sodium_base64_VARIANT_URLSAFE_NO_PADDING;
    }

    return base64VariantToInt(Base64Variant::STANDARD);
}

}