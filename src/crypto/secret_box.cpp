#include "secret_box.hpp"

#include <cstring> // std::memcpy
#include <sodium.h>

#include <util/crypto.hpp>
#include <defs/assert.hpp>
#include <defs/minimal_geode.hpp>

using namespace util::data;

SecretBox::SecretBox(bytevector key) {
    CRYPTO_REQUIRE(key.size() == crypto_secretbox_KEYBYTES, "provided key is too long or too short for SecretBox")

    this->key = reinterpret_cast<byte*>(sodium_malloc(
        crypto_secretbox_KEYBYTES
    ));

    CRYPTO_REQUIRE(this->key != nullptr, "sodium_malloc returned nullptr")

    std::memcpy(this->key, key.data(), crypto_secretbox_KEYBYTES);
}

SecretBox SecretBox::withPassword(const std::string_view pw) {
    auto key = util::crypto::simpleHash(pw);
    return SecretBox(key);
}

SecretBox::~SecretBox() {
    if (this->key) {
        sodium_free(this->key);
    }
}

size_t SecretBox::encryptInto(const byte* src, byte* dest, size_t size) {
    byte nonce[NONCE_LEN];
    util::crypto::secureRandom(nonce, NONCE_LEN);

    byte* ciphertext = dest + NONCE_LEN;
    CRYPTO_ERR_CHECK(crypto_secretbox_easy(ciphertext, src, size, nonce, key), "crypto_secretbox_easy failed")

    // prepend the nonce
    std::memcpy(dest, nonce, NONCE_LEN);

    return size + prefixLength();
}

size_t SecretBox::decryptInto(const byte* src, byte* dest, size_t size) {
    CRYPTO_REQUIRE(size >= prefixLength(), "message is too short")

    const byte* nonce = src;
    const byte* ciphertext = src + NONCE_LEN;

    size_t plaintextLength = size - prefixLength();
    size_t ciphertextLength = size - NONCE_LEN;

    CRYPTO_ERR_CHECK(crypto_secretbox_open_easy(dest, ciphertext, ciphertextLength, nonce, key), "crypto_secretbox_open_easy failed")

    return plaintextLength;
}

void SecretBox::setKey(const util::data::bytevector& src) {
    GLOBED_REQUIRE(src.size() == crypto_secretbox_KEYBYTES, "key size is too small or too big for SecretBox")
    setKey(src.data());
}

void SecretBox::setKey(const util::data::byte* src) {
    std::memcpy(this->key, src, crypto_secretbox_KEYBYTES);
}

void SecretBox::setPassword(const std::string_view pw) {
    auto key = util::crypto::simpleHash(pw);
    setKey(key);
}