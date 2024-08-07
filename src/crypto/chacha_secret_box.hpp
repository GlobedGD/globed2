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
    ChaChaSecretBox(util::data::bytevector key);
    ChaChaSecretBox(const ChaChaSecretBox&) = delete;
    ChaChaSecretBox& operator=(const ChaChaSecretBox&) = delete;
    ~ChaChaSecretBox();

    static ChaChaSecretBox withPassword(const std::string_view pw);

    Result<size_t> encryptInto(const util::data::byte* src, util::data::byte* dest, size_t size);
    Result<size_t> decryptInto(const util::data::byte* src, util::data::byte* dest, size_t size);

    Result<> setKey(const util::data::bytevector& src);
    void setKey(const util::data::byte* src);
    // hashes the password and initializes the secret key with the hash
    Result<> setPassword(const std::string_view pw);

private:
    util::data::byte* key = nullptr;
};
