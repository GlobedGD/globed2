#include "secret_box.hpp"
#include <util/crypto.hpp>
#include <util/rng.hpp>
#include <cstring> // std::memcpy

using namespace util::data;

SecretBox::SecretBox(bytevector key) {
    CRYPTO_SODIUM_INIT;
    
    CRYPTO_ASSERT(key.size() == crypto_secretbox_KEYBYTES, "provided key is too long or too short for SecretBox");

    this->key = reinterpret_cast<byte*>(sodium_malloc(
        crypto_secretbox_KEYBYTES
    ));

    CRYPTO_ASSERT(this->key != nullptr, "sodium_malloc returned nullptr");

    std::memcpy(this->key, key.data(), crypto_secretbox_KEYBYTES);
}

SecretBox SecretBox::withPassword(const std::string& pw) {
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
    util::rng::secureRandom(nonce, NONCE_LEN);

    byte* ciphertext = dest + NONCE_LEN;
    CRYPTO_ERR_CHECK(crypto_secretbox_easy(ciphertext, src, size, nonce, key), "crypto_secretbox_easy failed");

    // prepend the nonce
    std::memcpy(dest, nonce, NONCE_LEN);

    return size + PREFIX_LEN;
}

size_t SecretBox::decryptInto(const byte* src, byte* dest, size_t size) {
    CRYPTO_ASSERT(size >= PREFIX_LEN, "message is too short");

    const byte* nonce = src;
    const byte* ciphertext = src + NONCE_LEN;

    size_t plaintextLength = size - PREFIX_LEN;
    size_t ciphertextLength = size - NONCE_LEN;

    CRYPTO_ERR_CHECK(crypto_secretbox_open_easy(dest, ciphertext, ciphertextLength, nonce, key), "crypto_secretbox_open_easy failed");

    return plaintextLength;
}

void SecretBox::setKey(const util::data::bytevector& src) {
    GLOBED_ASSERT(src.size() == crypto_secretbox_KEYBYTES, "key size is too small or too big for SecretBox");
    setKey(src.data());
}

void SecretBox::setKey(const util::data::byte* src) {
    std::memcpy(this->key, src, crypto_secretbox_KEYBYTES);
}

void SecretBox::setPassword(const std::string& pw) {
    auto key = util::crypto::simpleHash(pw);
    setKey(key);
}