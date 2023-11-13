#pragma once
#include "base_box.hpp"
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

class CryptoBox : public BaseCryptoBox {
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

    // Get our public key as a bytearray instead of a borrowed pointer.
    util::data::bytearray<crypto_box_PUBLICKEYBYTES> extractPublicKey() noexcept;

    // The data is copied from src into a private member. You are responsible for freeing the source afterwards.
    // If the length is not equal to `crypto_box_PUBLICKEYBYTES` the behavior is undefined.
    // This precomputes the shared key and stores it for use in all future operations.
    void setPeerKey(util::data::byte* src);

    constexpr size_t nonceLength();
    constexpr size_t macLength();

    size_t encryptInto(const util::data::byte* src, util::data::byte* dest, size_t size);
    size_t decryptInto(const util::data::byte* src, util::data::byte* dest, size_t size);

private: // nuh uh
    util::data::byte* memBasePtr = nullptr;

    util::data::byte* secretKey;
    util::data::byte* publicKey;
    util::data::byte* peerPublicKey;

    util::data::byte* sharedKey;
};