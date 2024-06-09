#pragma once
#include "base_box.hpp"

class CryptoBox final : public BaseCryptoBox<CryptoBox> {
public:
    constexpr static size_t KEY_LEN = 32;
    constexpr static size_t NONCE_LEN = 24;
    constexpr static size_t MAC_LEN = 24;

    using BaseCryptoBox<CryptoBox>::PREFIX_LEN;

    static const char* algorithm();
    static const char* sodiumVersion();

    // Should be called exactly once at startup.
    static void initLibrary();

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

    size_t encryptInto(const util::data::byte* src, util::data::byte* dest, size_t size);
    size_t decryptInto(const util::data::byte* src, util::data::byte* dest, size_t size);

private: // nuh uh
    util::data::byte* memBasePtr = nullptr;

    util::data::byte* secretKey;
    util::data::byte* publicKey;
    util::data::byte* peerPublicKey;

    util::data::byte* sharedKey;
};
