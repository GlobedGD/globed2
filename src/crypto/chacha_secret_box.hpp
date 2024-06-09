#pragma once
#include "base_box.hpp"

/*
* ChaChaSecretBox - SecretBox with prefix chacha algo
*
* Algorithm - XChaCha2020Poly1305
* Tag implementation - prefix
*/

class ChaChaSecretBox final : public BaseCryptoBox<ChaChaSecretBox> {
public:
    static const size_t NONCE_LEN = 16;
    static const size_t KEY_LEN = 32;
    static const size_t MAC_LEN = 24;

    ChaChaSecretBox(util::data::bytevector key);
    ChaChaSecretBox(const ChaChaSecretBox&) = delete;
    ChaChaSecretBox& operator=(const ChaChaSecretBox&) = delete;
    ~ChaChaSecretBox();

    static ChaChaSecretBox withPassword(const std::string_view pw);

    size_t encryptInto(const util::data::byte* src, util::data::byte* dest, size_t size);
    size_t decryptInto(const util::data::byte* src, util::data::byte* dest, size_t size);

    void setKey(const util::data::bytevector& src);
    void setKey(const util::data::byte* src);
    // hashes the password and initializes the secret key with the hash
    void setPassword(const std::string_view pw);

private:
    util::data::byte* key = nullptr;
};
