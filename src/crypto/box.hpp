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
* Key exchange: X25519, encryption: XChaCha20/XSalsa20, authentication: Poly1305
*
* All methods (including the constructor) except for `getPublicKey` will throw an exception on failure.
*/

class CryptoBox : public BaseCryptoBox {
public:

// XSalsa20 is theoretically slower and less secure, but still possible to use by defining GLOBED_USE_XSALSA20
#ifdef GLOBED_USE_XSALSA20
    constexpr static size_t NONCE_LEN = crypto_box_NONCEBYTES;
    constexpr static size_t MAC_LEN = crypto_box_MACBYTES;

    constexpr static size_t KEY_LEN = crypto_box_PUBLICKEYBYTES;
    constexpr static size_t SECRET_KEY_LEN = crypto_box_SECRETKEYBYTES;
    constexpr static size_t SHARED_KEY_LEN = crypto_box_BEFORENMBYTES;

    constexpr static auto func_box_keypair = crypto_box_keypair;
    constexpr static auto func_box_beforenm = crypto_box_beforenm;
    constexpr static auto func_box_easy = crypto_box_easy_afternm;
    constexpr static auto func_box_open_easy = crypto_box_open_easy_afternm;
    constexpr static const char* ALGORITHM = "XSalsa20Poly1305";
#else
    constexpr static size_t NONCE_LEN = crypto_box_curve25519xchacha20poly1305_NONCEBYTES;
    constexpr static size_t MAC_LEN = crypto_box_curve25519xchacha20poly1305_MACBYTES;

    constexpr static size_t KEY_LEN = crypto_box_curve25519xchacha20poly1305_PUBLICKEYBYTES;
    constexpr static size_t SECRET_KEY_LEN = crypto_box_curve25519xchacha20poly1305_SECRETKEYBYTES;
    constexpr static size_t SHARED_KEY_LEN = crypto_box_curve25519xchacha20poly1305_BEFORENMBYTES;

    constexpr static auto func_box_keypair = crypto_box_curve25519xchacha20poly1305_keypair;
    constexpr static auto func_box_beforenm = crypto_box_curve25519xchacha20poly1305_beforenm;
    constexpr static auto func_box_easy = crypto_box_curve25519xchacha20poly1305_easy_afternm;
    constexpr static auto func_box_open_easy = crypto_box_curve25519xchacha20poly1305_open_easy_afternm;
    constexpr static const char* ALGORITHM = "XChaCha20Poly1305";
#endif

    constexpr static size_t PREFIX_LEN = NONCE_LEN + MAC_LEN;

    // Initialize this `CryptoBox`, optionally set peer's public key.
    CryptoBox(util::data::byte* peerKey = nullptr);
    CryptoBox(const CryptoBox&) = delete;
    CryptoBox& operator=(const CryptoBox&) = delete;
    ~CryptoBox();
    
    // Get our public key. The returned pointer lives as long as this `CryptoBox` object does.
    util::data::byte* getPublicKey() noexcept;

    // Get our public key as a bytearray instead of a borrowed pointer.
    util::data::bytearray<KEY_LEN> extractPublicKey() noexcept;

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