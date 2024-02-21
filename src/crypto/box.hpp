#pragma once
#include "base_box.hpp"

#include <sodium.h>

class CryptoBox final : public BaseCryptoBox {
public:

// XSalsa20 is theoretically slower and less secure, but still possible to use by defining GLOBED_USE_XSALSA20
#ifdef GLOBED_USE_XSALSA20
    constexpr static const char* ALGORITHM = "XSalsa20Poly1305";
# define CRYPTO_JOIN(arg) crypto_box_##arg
#else
    constexpr static const char* ALGORITHM = "XChaCha20Poly1305";
# define CRYPTO_JOIN(arg) crypto_box_curve25519xchacha20poly1305_##arg
#endif

    // i ain't retyping crypto_box_curve25519xchacha20poly1305_open_easy_afternm, okay?
    constexpr static size_t NONCE_LEN = CRYPTO_JOIN(NONCEBYTES);
    constexpr static size_t MAC_LEN = CRYPTO_JOIN(MACBYTES);

    constexpr static size_t KEY_LEN = CRYPTO_JOIN(PUBLICKEYBYTES);
    constexpr static size_t SECRET_KEY_LEN = CRYPTO_JOIN(SECRETKEYBYTES);
    constexpr static size_t SHARED_KEY_LEN = CRYPTO_JOIN(BEFORENMBYTES);

    constexpr static auto func_box_keypair = CRYPTO_JOIN(keypair);
    constexpr static auto func_box_beforenm = CRYPTO_JOIN(beforenm);
    constexpr static auto func_box_easy = CRYPTO_JOIN(easy_afternm);
    constexpr static auto func_box_open_easy = CRYPTO_JOIN(open_easy_afternm);

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
    // If the length is smaller than `CryptoBox::KEY_LEN` the behavior is undefined.
    // This precomputes the shared key and stores it for use in all future operations.
    void setPeerKey(const util::data::byte* src);

    constexpr size_t nonceLength() override;
    constexpr size_t macLength() override;

    size_t encryptInto(const util::data::byte* src, util::data::byte* dest, size_t size) override;
    size_t decryptInto(const util::data::byte* src, util::data::byte* dest, size_t size) override;

private: // nuh uh
    util::data::byte* memBasePtr = nullptr;

    util::data::byte* secretKey;
    util::data::byte* publicKey;
    util::data::byte* peerPublicKey;

    util::data::byte* sharedKey;
};
