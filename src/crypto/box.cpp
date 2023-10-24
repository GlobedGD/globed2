#include "box.hpp"
#include <stdexcept> // std::runtime_error
#include <cstring> // std::memcpy

#define CRYPTO_ASSERT(condition, message) GLOBED_ASSERT(condition, "crypto error: " message)
#define CRYPTO_ERR_CHECK(result, message) CRYPTO_ASSERT(result == 0, message)

using namespace util::data;

CryptoBox::CryptoBox(byte* key) {
    CRYPTO_ASSERT(sodium_init() != -1, "sodium_init failed");
    CRYPTO_ERR_CHECK(crypto_box_keypair(publicKey, secretKey), "crypto_box_keypair failed");

    if (key != nullptr) {
        setPeerKey(key);
    }
}

byte* CryptoBox::getPublicKey() {
    return publicKey;
}

void CryptoBox::setPeerKey(byte* key) {
    std::memcpy(peerPublicKey, key, crypto_box_PUBLICKEYBYTES);
}

bytevector CryptoBox::encrypt(const bytevector& data) {
    return encrypt(data.data(), data.size());
}

bytevector CryptoBox::encrypt(const byte* data, size_t size) {
    byte nonce[crypto_box_NONCEBYTES];
    randombytes(nonce, crypto_box_NONCEBYTES);

    bytevector ciphertext(size + crypto_box_MACBYTES);
    CRYPTO_ERR_CHECK(crypto_box_easy(ciphertext.data(), data, size, nonce, peerPublicKey, secretKey), "crypto_box_easy failed");

    bytevector result;
    result.insert(result.end(), nonce, nonce + crypto_box_NONCEBYTES);
    result.insert(result.end(), ciphertext.begin(), ciphertext.end());

    return result;
}

bytevector CryptoBox::encrypt(const std::string& data) {
    return encrypt(reinterpret_cast<const byte*>(data.c_str()), data.size());
}

bytevector CryptoBox::decrypt(const bytevector& data) {
    return decrypt(data.data(), data.size());
}

bytevector CryptoBox::decrypt(const byte* data, size_t size) {
    CRYPTO_ASSERT(size >= crypto_box_NONCEBYTES + crypto_box_MACBYTES, "message is too short");

    const byte* nonce = data;
    const byte* ciphertext = data + crypto_box_NONCEBYTES;
    size_t ciphertextLength = size - crypto_box_NONCEBYTES;
    size_t plaintextLength = size - crypto_box_NONCEBYTES - crypto_box_MACBYTES;

    bytevector plaintext(plaintextLength);
    CRYPTO_ERR_CHECK(crypto_box_open_easy(plaintext.data(), ciphertext, ciphertextLength, nonce, peerPublicKey, secretKey), "crypto_box_open_easy failed");

    return plaintext;
}

std::string CryptoBox::decryptToString(const bytevector& data) {
    auto vec = decrypt(data);
    return std::string(vec.begin(), vec.end());
}

std::string CryptoBox::decryptToString(const byte* data, size_t size) {
    auto vec = decrypt(data, size);
    return std::string(vec.begin(), vec.end());
}