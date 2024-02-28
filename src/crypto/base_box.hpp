#pragma once

#include <string>

#include <util/data.hpp>

/*
* This class contains no crypto implementation and is here just for boilerplate code.
* Implementers must override:
*
* size_t encryptInto(byte* src, byte* dest, size_t size)
*
* size_t decryptInto(byte* src, byte* dest, size_t size)
*
* constexpr size_t nonceLength();
*
* constexpr size_t macLength();
*/

class BaseCryptoBox {
public:
    // Encrypt `size` bytes from `src` into `dest`. Note: the `dest` buffer must be at least `size + prefixLength()` bytes big.
    // `src` and `dest` may overlap or be the same pointer. Returns the length of the encrypted data.
    virtual size_t encryptInto(const util::data::byte* src, util::data::byte* dest, size_t size) = 0;

    // Decrypt `size` bytes from `src` into `dest`. `src` and `dest` may overlap or be the same pointer. Returns the length of the plaintext data.
    virtual size_t decryptInto(const util::data::byte* src, util::data::byte* dest, size_t size) = 0;

    constexpr virtual size_t nonceLength() = 0;
    constexpr virtual size_t macLength() = 0;
    constexpr size_t prefixLength();

    /* Encryption */

    // Encrypt bytes from the string `src` into `dest`. Note: the `dest` buffer must be at least `size + prefixLength()` bytes big.
    // Returns the length of the encrypted data.
    size_t encryptInto(const std::string_view src, util::data::byte* dest);
    // Encrypt bytes from the bytevector `src` into `dest`. Note: the `dest` buffer must be at least `src.size() + prefixLength()` bytes big.
    // Returns the length of the encrypted data.
    size_t encryptInto(const util::data::bytevector& src, util::data::byte* dest);
    // Encrypt `size` bytes from `data` into itself. Note: the buffer must be at least `size + prefixLength()` bytes big.
    // Returns the length of the encrypted data.
    size_t encryptInPlace(util::data::byte* data, size_t size);

    // Encrypt bytes from bytevector `src` and return a bytevector with the encrypted data.
    util::data::bytevector encrypt(const util::data::bytevector& src);
    // Encrypt `size` bytes from byte buffer `src` and return a bytevector with the encrypted data.
    util::data::bytevector encrypt(const util::data::byte* src, size_t size);
    // Encrypt bytes from the string `src` and return a bytevector with the encrypted data.
    util::data::bytevector encrypt(const std::string_view src);

    /* Decryption */

    // Decrypt `size` bytes from `data` into itself. Returns the length of the plaintext data.
    size_t decryptInPlace(util::data::byte* data, size_t size);

    // Decrypt bytes from bytevector `src` and return a bytevector with the plaintext data.
    util::data::bytevector decrypt(const util::data::bytevector& src);
    // Decrypt `size` bytes from byte buffer `src` and return a bytevector with the plaintext data.
    util::data::bytevector decrypt(const util::data::byte* src, size_t size);

    // Decrypt bytes from bytevector `src` and return a string with the plaintext data.
    std::string decryptToString(const util::data::bytevector& src);
    // Decrypt `size` bytes from byte buffer `src` and return a string with the plaintext data.
    std::string decryptToString(const util::data::byte* src, size_t size);
};
