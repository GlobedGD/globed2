#include "secret_box.hpp"

#include <cstring> // std::memcpy

#include <util/crypto.hpp>
#include <defs/assert.hpp>
#include <defs/minimal_geode.hpp>

using namespace util::data;

SecretBox::SecretBox(bytevector key) {
    CRYPTO_REQUIRE(key.size() == KEY_LEN, "provided key is too long or too short for SecretBox")

    this->key = reinterpret_cast<byte*>(sodium_malloc(
        KEY_LEN
    ));

    CRYPTO_REQUIRE(this->key != nullptr, "sodium_malloc returned nullptr")

    std::memcpy(this->key, key.data(), KEY_LEN);
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

constexpr size_t SecretBox::nonceLength() {
    return NONCE_LEN;
}

constexpr size_t SecretBox::macLength() {
    return MAC_LEN;
}

size_t SecretBox::encryptInto(const byte* src, byte* dest, size_t size) {
    byte nonce[NONCE_LEN];
    util::crypto::secureRandom(nonce, NONCE_LEN);

    byte* mac = dest + NONCE_LEN;
    byte* ciphertext = mac + MAC_LEN;

    CRYPTO_ERR_CHECK(crypto_secretbox_xchacha20poly1305_detached(ciphertext, mac, src, size, nonce, key), "crypto_secretbox_detached failed")

    // prepend the nonce
    std::memcpy(dest, nonce, NONCE_LEN);

    return size + PREFIX_LEN;
}

size_t SecretBox::decryptInto(const byte* src, byte* dest, size_t size) {
    CRYPTO_REQUIRE(size >= PREFIX_LEN, "message is too short")

    size_t plaintextLength = size - PREFIX_LEN;

    const byte* nonce = src;
    const byte* mac = src + NONCE_LEN;
    const byte* ciphertext = mac + MAC_LEN;

    CRYPTO_ERR_CHECK(crypto_secretbox_xchacha20poly1305_open_detached(dest, ciphertext, mac, plaintextLength, nonce, key), "crypto_secretbox_open_easy failed")

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