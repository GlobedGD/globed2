#include "box.hpp"

#include <cstring> // std::memcpy

#include <util/crypto.hpp>
#include <defs/assert.hpp>
#include <defs/minimal_geode.hpp>

using namespace util::data;

CryptoBox::CryptoBox(byte* key) {
    memBasePtr = reinterpret_cast<byte*>(sodium_malloc(
        KEY_LEN * 2 + // publicKey, peerPublicKey
        SECRET_KEY_LEN + // secretKey
        SHARED_KEY_LEN // sharedKey
    ));

    CRYPTO_REQUIRE(memBasePtr != nullptr, "sodium_malloc returned nullptr")

    secretKey = memBasePtr; // base + 0
    publicKey = secretKey + SECRET_KEY_LEN; // base + 32
    peerPublicKey = publicKey + KEY_LEN; // base + 64
    sharedKey = peerPublicKey + KEY_LEN; // base + 96

    CRYPTO_ERR_CHECK(func_box_keypair(publicKey, secretKey), "func_box_keypair failed")

    if (key != nullptr) {
        this->setPeerKey(key);
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

bytearray<CryptoBox::KEY_LEN> CryptoBox::extractPublicKey() noexcept {
    bytearray<KEY_LEN> out;
    std::memcpy(out.data(), publicKey, KEY_LEN);
    return out;
}

void CryptoBox::setPeerKey(const byte* key) {
    std::memcpy(peerPublicKey, key, KEY_LEN);

    CRYPTO_ERR_CHECK(func_box_beforenm(sharedKey, peerPublicKey, secretKey), "func_box_beforenm failed")
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
    CRYPTO_ERR_CHECK(func_box_easy(ciphertext, src, size, nonce, sharedKey), "func_box_easy failed")

    // prepend the nonce
    std::memcpy(dest, nonce, NONCE_LEN);

    return size + PREFIX_LEN;
}

size_t CryptoBox::decryptInto(const util::data::byte* src, util::data::byte* dest, size_t size) {
    CRYPTO_REQUIRE(size >= PREFIX_LEN, "message is too short")

    const byte* nonce = src;
    const byte* ciphertext = src + NONCE_LEN;

    size_t plaintextLength = size - PREFIX_LEN;
    size_t ciphertextLength = size - NONCE_LEN;

    CRYPTO_ERR_CHECK(func_box_open_easy(dest, ciphertext, ciphertextLength, nonce, sharedKey), "func_box_open_easy failed")

    return plaintextLength;
}