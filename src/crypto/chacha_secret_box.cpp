#include "chacha_secret_box.hpp"

#include <cstring> // std::memcpy
#include <sodium.h>

#include <util/crypto.hpp>
#include <defs/assert.hpp>
#include <defs/minimal_geode.hpp>

using namespace util::data;

ChaChaSecretBox::ChaChaSecretBox(bytevector key) {
    CRYPTO_REQUIRE(key.size() == KEY_LEN, "provided key is too long or too short for ChaChaSecretBox")

    this->key = reinterpret_cast<byte*>(sodium_malloc(
        KEY_LEN
    ));

    CRYPTO_REQUIRE(this->key != nullptr, "sodium_malloc returned nullptr")

    std::memcpy(this->key, key.data(), KEY_LEN);
}

ChaChaSecretBox ChaChaSecretBox::withPassword(const std::string_view pw) {
    auto key = util::crypto::simpleHash(pw);
    return ChaChaSecretBox(key);
}

ChaChaSecretBox::~ChaChaSecretBox() {
    if (this->key) {
        sodium_free(this->key);
    }
}

size_t ChaChaSecretBox::encryptInto(const byte* src, byte* dest, size_t size) {
    byte nonce[NONCE_LEN];
    util::crypto::secureRandom(nonce, NONCE_LEN);

    byte* mac = dest + NONCE_LEN;
    byte* ciphertext = mac + MAC_LEN;

    CRYPTO_ERR_CHECK(crypto_secretbox_xchacha20poly1305_detached(ciphertext, mac, src, size, nonce, key), "crypto_secretbox_xchacha20poly1305_detached failed")

    // prepend the nonce
    std::memcpy(dest, nonce, NONCE_LEN);

    return size + prefixLength();
}

size_t ChaChaSecretBox::decryptInto(const byte* src, byte* dest, size_t size) {
    CRYPTO_REQUIRE(size >= prefixLength(), "message is too short")

    size_t plaintextLength = size - prefixLength();

    const byte* nonce = src;
    const byte* mac = src + NONCE_LEN;
    const byte* ciphertext = mac + MAC_LEN;

    CRYPTO_ERR_CHECK(crypto_secretbox_xchacha20poly1305_open_detached(dest, ciphertext, mac, plaintextLength, nonce, key), "crypto_secretbox_xchacha20poly1305_open_detached failed")

    return plaintextLength;
}

void ChaChaSecretBox::setKey(const util::data::bytevector& src) {
    GLOBED_REQUIRE(src.size() == crypto_secretbox_KEYBYTES, "key size is too small or too big for ChaChaSecretBox")
    setKey(src.data());
}

void ChaChaSecretBox::setKey(const util::data::byte* src) {
    std::memcpy(this->key, src, crypto_secretbox_KEYBYTES);
}

void ChaChaSecretBox::setPassword(const std::string_view pw) {
    auto key = util::crypto::simpleHash(pw);
    setKey(key);
}