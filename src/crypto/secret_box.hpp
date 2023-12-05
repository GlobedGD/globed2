#pragma once
#include "base_box.hpp"

#include <sodium.h>

/*
* SecretBox - a class similar to CryptoBox, but instead of using public key cryptography,
* uses a single secret key (or derives it from a passphrase) for data encryption.
*/

class SecretBox : public BaseCryptoBox {
public:
    static const size_t NONCE_LEN = crypto_secretbox_NONCEBYTES;
    static const size_t MAC_LEN = crypto_secretbox_MACBYTES;
    static const size_t PREFIX_LEN = NONCE_LEN + MAC_LEN;

    SecretBox(util::data::bytevector key);
    SecretBox(const SecretBox&) = delete;
    SecretBox& operator=(const SecretBox&) = delete;
    ~SecretBox();

    static SecretBox withPassword(const std::string& pw);

    constexpr size_t nonceLength();
    constexpr size_t macLength();
    using BaseCryptoBox::prefixLength;

    size_t encryptInto(const util::data::byte* src, util::data::byte* dest, size_t size);
    size_t decryptInto(const util::data::byte* src, util::data::byte* dest, size_t size);

    void setKey(const util::data::bytevector& src);
    void setKey(const util::data::byte* src);
    // hashes the password and initializes the secret key with the hash
    void setPassword(const std::string& pw);

private:
    util::data::byte* key = nullptr;
};