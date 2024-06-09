#pragma once
#include "base_box.hpp"

/*
* SecretBox - a class similar to CryptoBox, but instead of using public key cryptography,
* uses a single secret key (or derives it from a passphrase) for data encryption.
*/

class SecretBox final : public BaseCryptoBox<SecretBox> {
public:
    static const size_t NONCE_LEN = 24;
    static const size_t MAC_LEN = 16;
    static const size_t KEY_LEN = 32;

    SecretBox(util::data::bytevector key);
    SecretBox(const SecretBox&) = delete;
    SecretBox& operator=(const SecretBox&) = delete;
    ~SecretBox();

    static SecretBox withPassword(const std::string_view pw);

    size_t encryptInto(const util::data::byte* src, util::data::byte* dest, size_t size);
    size_t decryptInto(const util::data::byte* src, util::data::byte* dest, size_t size);

    void setKey(const util::data::bytevector& src);
    void setKey(const util::data::byte* src);
    // hashes the password and initializes the secret key with the hash
    void setPassword(const std::string_view pw);

private:
    util::data::byte* key = nullptr;
};
