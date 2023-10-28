#pragma once
#include <defs.hpp>
#include <util/data.hpp>

#include <sodium.h>
#include <string>

/*
* CryptoBox - A class for secure data encryption/decryption using libsodium
*
* The algorithms used are (as per https://libsodium.gitbook.io/doc/public-key_cryptography/authenticated_encryption#algorithm-details)
*
* Key exchange: X25519, encryption: XSalsa20, authentication: Poly1305
*
* All methods (including the constructor) except for `getPublicKey` will throw an exception on failure.
*/

class CryptoBox {
public:
    static const size_t NONCE_LEN = crypto_box_NONCEBYTES;
    static const size_t MAC_LEN = crypto_box_MACBYTES;
    static const size_t PREFIX_LEN = NONCE_LEN + MAC_LEN;

    // Initialize this `CryptoBox`, optionally set peer's public key.
    CryptoBox(util::data::byte* peerKey = nullptr);
    CryptoBox(const CryptoBox&) = delete;
    CryptoBox& operator=(const CryptoBox&) = delete;
    ~CryptoBox();
    
    // Get our public key. The returned pointer lives as long as this `CryptoBox` object does.
    util::data::byte* getPublicKey() noexcept;

    // The data is copied from src into a private member. You are responsible for freeing the source afterwards.
    // If the length is not equal to `crypto_box_PUBLICKEYBYTES` the behavior is undefined.
    // This precomputes the shared key and stores it for use in all future operations.
    void setPeerKey(util::data::byte* src);

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
    util::data::byte* memBasePtr = nullptr;

    util::data::byte* secretKey;
    util::data::byte* publicKey;
    util::data::byte* peerPublicKey;

    util::data::byte* sharedKey;
};