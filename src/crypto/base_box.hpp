#pragma once

#include <string>
#include <cstring> // std::memmove

#include <defs/minimal_geode.hpp>
#include <defs/assert.hpp>
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

// this does not work bc c++ is shit
template <typename T>
concept CBox = requires(T box, const util::data::byte* src, util::data::byte* dest, size_t size) {
    std::is_class_v<T>;
    { box.encryptInto(src, dest, size) } -> std::convertible_to<size_t>;
    { box.decryptInto(src, dest, size) } -> std::convertible_to<size_t>;
};

template <typename Derived>
class BaseCryptoBox {
protected:
    using byte = util::data::byte;
    using bytevector = util::data::bytevector;

public:
    // Preferrably we should define those separately for each subclass, but it does not compile on MSVC.
    constexpr static size_t KEY_LEN = 32;
    constexpr static size_t NONCE_LEN = 24;
    constexpr static size_t MAC_LEN = 16;

    constexpr static size_t PREFIX_LEN = NONCE_LEN + MAC_LEN;

    constexpr static size_t prefixLength() {
        return PREFIX_LEN;
    }

    constexpr static size_t nonceLength() {
        return NONCE_LEN;
    }

    constexpr static size_t macLength() {
        return MAC_LEN;
    }

    constexpr static size_t keyLength() {
        return KEY_LEN;
    }

    /* Encryption */

    // Encrypt bytes from the string `src` into `dest`. Note: the `dest` buffer must be at least `size + prefixLength()` bytes big.
    // Returns the length of the encrypted data.
    size_t encryptInto(const std::string_view src, byte* dest) {
        return static_cast<Derived*>(this)->encryptInto(reinterpret_cast<const byte*>(src.data()), dest, src.size()).unwrap();
    }

    // Encrypt bytes from the bytevector `src` into `dest`. Note: the `dest` buffer must be at least `src.size() + prefixLength()` bytes big.
    // Returns the length of the encrypted data.
    size_t encryptInto(const bytevector& src, byte* dest) {
        return static_cast<Derived*>(this)->encryptInto(src.data(), dest, src.size()).unwrap();
    }

    // Encrypt `size` bytes from `data` into itself. Note: the buffer must be at least `size + prefixLength()` bytes big.
    // Returns the length of the encrypted data.
    size_t encryptInPlace(byte* data, size_t size) {
        return static_cast<Derived*>(this)->encryptInto(data, data, size).unwrap();
    }

    // Encrypt bytes from bytevector `src` and return a bytevector with the encrypted data.
    bytevector encrypt(const bytevector& src) {
        return encrypt(src.data(), src.size());
    }

    // Encrypt `size` bytes from byte buffer `src` and return a bytevector with the encrypted data.
    bytevector encrypt(const byte* src, size_t size) {
        bytevector output(size + prefixLength());
        (void) static_cast<Derived*>(this)->encryptInto(src, output.data(), size);
        return output;
    }

    // Encrypt bytes from the string `src` and return a bytevector with the encrypted data.
    bytevector encrypt(const std::string_view src) {
        return encrypt(reinterpret_cast<const byte*>(src.data()), src.size());
    }

    /* Decryption */

    // Decrypt `size` bytes from `data` into itself. Returns the length of the plaintext data.
    Result<size_t> decryptInPlace(byte* data, size_t size) {
        // overwriting nonce causes decryption to break
        // so we offset the destination by NONCE_LEN and then move it back
        GLOBED_UNWRAP_INTO(static_cast<Derived*>(this)->decryptInto(data, data + nonceLength(), size), size_t plaintext_size);

        std::memmove(data, data + nonceLength(), plaintext_size);

        return Ok(plaintext_size);
    }

    // Decrypt bytes from bytevector `src` and return a bytevector with the plaintext data.
    Result<bytevector> decrypt(const bytevector& src) {
        return decrypt(src.data(), src.size());
    }

    // Decrypt `size` bytes from byte buffer `src` and return a bytevector with the plaintext data.
    Result<bytevector> decrypt(const byte* src, size_t size) {
        if (size < prefixLength()) throw std::runtime_error("data too short");

        size_t plaintextLength = size - prefixLength();

        bytevector plaintext(plaintextLength);
        GLOBED_UNWRAP(static_cast<Derived*>(this)->decryptInto(src, plaintext.data(), size));

        return Ok(std::move(plaintext));
    }

    // Decrypt bytes from bytevector `src` and return a string with the plaintext data.
    Result<std::string> decryptToString(const bytevector& src) {
        GLOBED_UNWRAP_INTO(decrypt(src), auto vec);
        return Ok(std::string(vec.begin(), vec.end()));
    }

    // Decrypt `size` bytes from byte buffer `src` and return a string with the plaintext data.
    Result<std::string> decryptToString(const byte* src, size_t size) {
        GLOBED_UNWRAP_INTO(decrypt(src, size), auto vec);
        return Ok(std::string(vec.begin(), vec.end()));
    }
};
