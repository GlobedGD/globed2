#pragma once
#include "base_box.hpp"

#include <sodium.h>

/*
* ChaChaSecretBox - SecretBox with prefix chacha algo
*
* Algorithm - XChaCha2020Poly1305
* Tag implementation - prefix
*/

class ChaChaSecretBox final : public BaseCryptoBox {
public:
    static const size_t NONCE_LEN = crypto_secretbox_xchacha20poly1305_NONCEBYTES;
    static const size_t MAC_LEN = crypto_secretbox_xchacha20poly1305_MACBYTES;
    static const size_t KEY_LEN = crypto_secretbox_xchacha20poly1305_KEYBYTES;
    static const size_t PREFIX_LEN = NONCE_LEN + MAC_LEN;

    ChaChaSecretBox(util::data::bytevector key);
    ChaChaSecretBox(const ChaChaSecretBox&) = delete;
    ChaChaSecretBox& operator=(const ChaChaSecretBox&) = delete;
    ~ChaChaSecretBox();

    static ChaChaSecretBox withPassword(const std::string_view pw);

    constexpr size_t nonceLength() override;
    constexpr size_t macLength() override;
    using BaseCryptoBox::prefixLength;

    size_t encryptInto(const util::data::byte* src, util::data::byte* dest, size_t size) override;
    size_t decryptInto(const util::data::byte* src, util::data::byte* dest, size_t size) override;

    void setKey(const util::data::bytevector& src);
    void setKey(const util::data::byte* src);
    // hashes the password and initializes the secret key with the hash
    void setPassword(const std::string_view pw);

private:
    util::data::byte* key = nullptr;
};
