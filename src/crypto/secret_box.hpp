#pragma once
#include "base_box.hpp"

/*
* SecretBox - a class similar to CryptoBox, but instead of using public key cryptography,
* uses a single secret key (or derives it from a passphrase) for data encryption.
*/

class SecretBox final : public BaseCryptoBox<SecretBox> {
public:
    SecretBox(util::data::bytevector key);
    SecretBox(const SecretBox&) = delete;
    SecretBox& operator=(const SecretBox&) = delete;
    ~SecretBox();

    static SecretBox withPassword(const std::string_view pw);

    Result<size_t> encryptInto(const util::data::byte* src, util::data::byte* dest, size_t size);
    Result<size_t> decryptInto(const util::data::byte* src, util::data::byte* dest, size_t size);

    Result<> setKey(const util::data::bytevector& src);
    void setKey(const util::data::byte* src);
    // hashes the password and initializes the secret key with the hash
    Result<> setPassword(const std::string_view pw);

private:
    util::data::byte* key = nullptr;
};
