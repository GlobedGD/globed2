#pragma once

#include <string>
#include <cstring> // std::memmove

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
    { T::MAC_LEN } -> std::convertible_to<size_t>;
    { T::NONCE_LEN } -> std::convertible_to<size_t>;
    { T::KEY_LEN } -> std::convertible_to<size_t>;
};

template <typename Derived>
class BaseCryptoBox {
protected:
    using byte = util::data::byte;
    using bytevector = util::data::bytevector;

public:
    constexpr static size_t PREFIX_LEN = Derived::NONCE_LEN + Derived::MAC_LEN;

    constexpr static size_t prefixLength() {
        return PREFIX_LEN;
    }

    constexpr static size_t nonceLength() {
        return Derived::NONCE_LEN;
    }

    constexpr static size_t macLength() {
        return Derived::MAC_LEN;
    }

    constexpr static size_t keyLength() {
        return Derived::KEY_LEN;
    }

    /* Encryption */

    // Encrypt bytes from the string `src` into `dest`. Note: the `dest` buffer must be at least `size + prefixLength()` bytes big.
    // Returns the length of the encrypted data.
    size_t encryptInto(const std::string_view src, byte* dest) {
        return static_cast<Derived*>(this)->encryptInto(reinterpret_cast<const byte*>(src.data()), dest, src.size());
    }

    // Encrypt bytes from the bytevector `src` into `dest`. Note: the `dest` buffer must be at least `src.size() + prefixLength()` bytes big.
    // Returns the length of the encrypted data.
    size_t encryptInto(const bytevector& src, byte* dest) {
        return static_cast<Derived*>(this)->encryptInto(src.data(), dest, src.size());
    }

    // Encrypt `size` bytes from `data` into itself. Note: the buffer must be at least `size + prefixLength()` bytes big.
    // Returns the length of the encrypted data.
    size_t encryptInPlace(byte* data, size_t size) {
        return static_cast<Derived*>(this)->encryptInto(data, data, size);
    }

    // Encrypt bytes from bytevector `src` and return a bytevector with the encrypted data.
    bytevector encrypt(const bytevector& src) {
        return encrypt(src.data(), src.size());
    }

    // Encrypt `size` bytes from byte buffer `src` and return a bytevector with the encrypted data.
    bytevector encrypt(const byte* src, size_t size) {
        bytevector output(size + prefixLength());
        static_cast<Derived*>(this)->encryptInto(src, output.data(), size);
        return output;
    }

    // Encrypt bytes from the string `src` and return a bytevector with the encrypted data.
    bytevector encrypt(const std::string_view src) {
        return encrypt(reinterpret_cast<const byte*>(src.data()), src.size());
    }

    /* Decryption */

    // Decrypt `size` bytes from `data` into itself. Returns the length of the plaintext data.
    size_t decryptInPlace(byte* data, size_t size) {
        // overwriting nonce causes decryption to break
        // so we offset the destination by NONCE_LEN and then move it back
        size_t plaintext_size = static_cast<Derived*>(this)->decryptInto(data, data + nonceLength(), size);

        std::memmove(data, data + nonceLength(), plaintext_size);

        return plaintext_size;
    }

    // Decrypt bytes from bytevector `src` and return a bytevector with the plaintext data.
    bytevector decrypt(const bytevector& src) {
        return decrypt(src.data(), src.size());
    }

    // Decrypt `size` bytes from byte buffer `src` and return a bytevector with the plaintext data.
    bytevector decrypt(const byte* src, size_t size) {
        size_t plaintextLength = size - prefixLength();

        bytevector plaintext(plaintextLength);
        static_cast<Derived*>(this)->decryptInto(src, plaintext.data(), size);

        return plaintext;
    }

    // Decrypt bytes from bytevector `src` and return a string with the plaintext data.
    std::string decryptToString(const bytevector& src) {
        auto vec = decrypt(src);
        return std::string(vec.begin(), vec.end());
    }

    // Decrypt `size` bytes from byte buffer `src` and return a string with the plaintext data.
    std::string decryptToString(const byte* src, size_t size) {
        auto vec = decrypt(src, size);
        return std::string(vec.begin(), vec.end());
    }
};
