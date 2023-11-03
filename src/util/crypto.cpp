#include "crypto.hpp"
#include <sodium.h>

using namespace util::data;

namespace util::crypto {

bytevector pwHash(const std::string& input) {
    return pwHash(reinterpret_cast<const byte*>(input.c_str()), input.size());
}

bytevector pwHash(const bytevector& input) {
    return pwHash(reinterpret_cast<const byte*>(input.data()), input.size());
}

bytevector pwHash(const byte* input, size_t len) {
    CRYPTO_SODIUM_INIT;

    bytevector out(crypto_pwhash_SALTBYTES + crypto_secretbox_KEYBYTES);
    byte* salt = out.data();
    byte* key = out.data() + crypto_pwhash_SALTBYTES;
    randombytes_buf(salt, crypto_pwhash_SALTBYTES);

    CRYPTO_ERR_CHECK(crypto_pwhash(
        key,
        crypto_secretbox_KEYBYTES,
        reinterpret_cast<const char*>(input),
        len,
        salt,
        crypto_pwhash_OPSLIMIT_INTERACTIVE,
        crypto_pwhash_MEMLIMIT_INTERACTIVE,
        crypto_pwhash_ALG_DEFAULT
    ), "crypto_pwhash failed");

    return out;
}

bytevector simpleHash(const std::string& input) {
    return simpleHash(reinterpret_cast<const byte*>(input.c_str()), input.size());
}

bytevector simpleHash(const bytevector& input) {
    return simpleHash(reinterpret_cast<const byte*>(input.data()), input.size());
}

bytevector simpleHash(const byte* input, size_t size) {
    bytevector out(crypto_generichash_BYTES);
    CRYPTO_ERR_CHECK(crypto_generichash(
        out.data(),
        crypto_generichash_BYTES,
        input, size,
        nullptr, 0
    ), "crypto_generichash failed");

    return out;
}

}