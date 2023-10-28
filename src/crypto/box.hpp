#pragma once
#include <defs.hpp>
#include <util/data.hpp>

#include <sodium.h>
#include <string>

/*
* CryptoBox - A class for secure data encryption/decryption using libsodium
*
* Uses `crypto_box_easy` and `crypto_box_open_easy` for en/decryption, and `crypto_box_keypair` for key generation.
* The algorithms used are (as per https://libsodium.gitbook.io/doc/public-key_cryptography/authenticated_encryption#algorithm-details)
*
* Key exchange: X25519
* Encryption: XSalsa20
* Authentication: Poly1305
*
* All methods (including the constructor) except for `getPublicKey` and `setPeerKey` will throw an exception on failure.
*/

class CryptoBox {
public:
    static const size_t NONCE_LEN = crypto_box_NONCEBYTES;
    static const size_t MAC_LEN = crypto_box_MACBYTES;
    static const size_t PREFIX_LEN = NONCE_LEN + MAC_LEN;

    CryptoBox(util::data::byte* peerKey = nullptr);
    
    // Get our public key. The returned pointer lives as long as this `CryptoBox` object does.
    util::data::byte* getPublicKey() noexcept;

    // The data is copied from src into a private member. You are responsible for freeing the source afterwards.
    // If the length is not equal to `crypto_box_PUBLICKEYBYTES` the behavior is undefined.
    void setPeerKey(util::data::byte* src) noexcept;

    /* Encryption */

    // Encrypt `size` bytes from `src` into `dest`. Note: the `dest` buffer must be at least `size + CryptoBox::PREFIX_LEN` bytes big.
    // `src` and `dest` can overlap or be the same pointer. Returns the length of the encrypted data.
    size_t encryptInto(const util::data::byte* src, util::data::byte* dest, size_t size);

    // Encrypt bytes from the string `src` into `dest`. Note: the `dest` buffer must be at least `size + CryptoBox::PREFIX_LEN` bytes big.
    // Returns the length of the encrypted data.
    size_t encryptInto(const std::string& src, util::data::byte* dest);
    // Encrypt bytes from the bytevector `src` into `dest`. Note: the `dest` buffer must be at least `src.size() + CryptoBox::PREFIX_LEN` bytes big.
    // Returns the length of the encrypted data.
    size_t encryptInto(const util::data::bytevector& src, util::data::byte* dest);
    // Encrypt `size` bytes from `data` into itself. Note: the buffer must be at least `size + CryptoBox::PREFIX_LEN` bytes big.
    // Returns the length of the encrypted data.
    size_t encryptInPlace(util::data::byte* data, size_t size);

    // Encrypt bytes from bytevector `src` and return a bytevector with the encrypted data.
    util::data::bytevector encrypt(const util::data::bytevector& src);
    // Encrypt `size` bytes from byte array `src` and return a bytevector with the encrypted data.
    util::data::bytevector encrypt(const util::data::byte* src, size_t size);
    // Encrypt bytes from the string `src` and return a bytevector with the encrypted data.
    util::data::bytevector encrypt(const std::string& src);

    /* Decryption */

    // Decrypt `size` bytes from `src` into `dest`. `src` and `dest` can overlap or be the same pointer. Returns the length of the plaintext data.
    size_t decryptInto(const util::data::byte* src, util::data::byte* dest, size_t size);
    // Decrypt `size` bytes from `data` into itself. Returns the length of the plaintext data.
    size_t decryptInPlace(util::data::byte* data, size_t size);

    // Decrypt bytes from bytevector `src` and return a bytevector with the plaintext data.
    util::data::bytevector decrypt(const util::data::bytevector& src);
    // Decrypt `size` bytes from byte array `src` and return a bytevector with the plaintext data.
    util::data::bytevector decrypt(const util::data::byte* src, size_t size);

    // Decrypt bytes from bytevector `src` and return a string with the plaintext data.
    std::string decryptToString(const util::data::bytevector& src);
    // Decrypt `size` bytes from byte array `src` and return a string with the plaintext data.
    std::string decryptToString(const util::data::byte* src, size_t size);

private: // nuh uh
    util::data::byte secretKey[crypto_box_SECRETKEYBYTES];
    util::data::byte publicKey[crypto_box_PUBLICKEYBYTES];
    util::data::byte peerPublicKey[crypto_box_PUBLICKEYBYTES];
};