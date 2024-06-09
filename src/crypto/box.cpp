#include "box.hpp"

#include <cstring> // std::memcpy
#include <sodium.h>

#include <util/crypto.hpp>
#include <defs/assert.hpp>
#include <defs/minimal_geode.hpp>

#include <util/debug.hpp>

using namespace util::data;

// XSalsa20 is theoretically slower and less secure, but still possible to use by defining GLOBED_USE_XSALSA20
#ifdef GLOBED_USE_XSALSA20
constexpr static const char* ALGORITHM = "XSalsa20Poly1305";
# define CRYPTO_JOIN(arg) crypto_box_##arg
#else
constexpr static const char* ALGORITHM = "XChaCha20Poly1305";
# define CRYPTO_JOIN(arg) crypto_box_curve25519xchacha20poly1305_##arg
#endif

// i ain't retyping crypto_box_curve25519xchacha20poly1305_open_easy_afternm, okay?
constexpr size_t NONCE_LEN = CRYPTO_JOIN(NONCEBYTES);
constexpr size_t MAC_LEN = CRYPTO_JOIN(MACBYTES);

constexpr size_t KEY_LEN = CRYPTO_JOIN(PUBLICKEYBYTES);
constexpr size_t SECRET_KEY_LEN = CRYPTO_JOIN(SECRETKEYBYTES);
constexpr size_t SHARED_KEY_LEN = CRYPTO_JOIN(BEFORENMBYTES);

constexpr auto func_box_keypair = CRYPTO_JOIN(keypair);
constexpr auto func_box_beforenm = CRYPTO_JOIN(beforenm);
constexpr auto func_box_easy = CRYPTO_JOIN(easy_afternm);
constexpr auto func_box_open_easy = CRYPTO_JOIN(open_easy_afternm);

constexpr static size_t PREFIX_LEN = NONCE_LEN + MAC_LEN;

void CryptoBox::initLibrary() {
    // sodium_init returns 0 on success, 1 if already initialized, -1 on fail
    GLOBED_REQUIRE(sodium_init() != -1, "sodium_init failed")

    // if there is a logic error in the crypto code, this lambda will be called
    sodium_set_misuse_handler([] {
        log::error("sodium_misuse called. we are officially screwed.");
        util::debug::suicide();
    });
}

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

const char* CryptoBox::algorithm() {
    return ALGORITHM;
}

const char* CryptoBox::sodiumVersion() {
    return SODIUM_VERSION_STRING;
}