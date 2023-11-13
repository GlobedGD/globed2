// this is incredibly overoptimized xd

#include "box.hpp"
#include <util/crypto.hpp>
#include <stdexcept> // std::runtime_error
#include <cstring> // std::memcpy

using namespace util::data;

CryptoBox::CryptoBox(byte* key) {
    CRYPTO_SODIUM_INIT;

    memBasePtr = reinterpret_cast<byte*>(sodium_malloc(
        crypto_box_PUBLICKEYBYTES * 2 + // publicKey, peerPublicKey
        crypto_box_SECRETKEYBYTES + // secretKey
        crypto_box_BEFORENMBYTES // sharedKey
    ));

    CRYPTO_ASSERT(memBasePtr != nullptr, "sodium_malloc returned nullptr");

    secretKey = memBasePtr; // base + 0
    publicKey = secretKey + crypto_box_SECRETKEYBYTES; // base + 32
    peerPublicKey = publicKey + crypto_box_PUBLICKEYBYTES; // base + 64
    sharedKey = peerPublicKey + crypto_box_PUBLICKEYBYTES; // base + 96

    CRYPTO_ERR_CHECK(crypto_box_keypair(publicKey, secretKey), "crypto_box_keypair failed");

    if (key != nullptr) {
        setPeerKey(key);
    }
}

CryptoBox::~CryptoBox() {
    if (memBasePtr) {
        sodium_free(memBasePtr);
    }
}

byte* CryptoBox::getPublicKey() noexcept {
    return publicKey;
}

bytearray<crypto_box_PUBLICKEYBYTES> CryptoBox::extractPublicKey() noexcept {
    bytearray<crypto_box_PUBLICKEYBYTES> out;
    std::memcpy(out.data(), publicKey, crypto_box_PUBLICKEYBYTES);
    return out;
}

void CryptoBox::setPeerKey(byte* key) {
    std::memcpy(peerPublicKey, key, crypto_box_PUBLICKEYBYTES);
    CRYPTO_ERR_CHECK(crypto_box_beforenm(sharedKey, peerPublicKey, secretKey), "crypto_box_beforenm failed");
}

constexpr size_t CryptoBox::nonceLength() {
    return NONCE_LEN;
}

constexpr size_t CryptoBox::macLength() {
    return MAC_LEN;
}

size_t CryptoBox::encryptInto(const byte* src, byte* dest, size_t size) {
    byte nonce[NONCE_LEN];
    util::crypto::secureRandom(nonce, NONCE_LEN);

    byte* ciphertext = dest + NONCE_LEN;
    CRYPTO_ERR_CHECK(crypto_box_easy_afternm(ciphertext, src, size, nonce, sharedKey), "crypto_box_easy_afternm failed");

    // prepend the nonce
    std::memcpy(dest, nonce, NONCE_LEN);

    return size + PREFIX_LEN;
}


size_t CryptoBox::decryptInto(const util::data::byte* src, util::data::byte* dest, size_t size) {
    CRYPTO_ASSERT(size >= PREFIX_LEN, "message is too short");

    const byte* nonce = src;
    const byte* ciphertext = src + NONCE_LEN;

    size_t plaintextLength = size - PREFIX_LEN;
    size_t ciphertextLength = size - NONCE_LEN;

    CRYPTO_ERR_CHECK(crypto_box_open_easy_afternm(dest, ciphertext, ciphertextLength, nonce, sharedKey), "crypto_box_open_easy_afternm failed");

    return plaintextLength;
}