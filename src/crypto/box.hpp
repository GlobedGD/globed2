#pragma once
#include <defs.hpp>
#include <util/data.hpp>

#include <sodium.h>
#include <string>

/*
* CryptoBox - A class for secure data encryption/decryption using libsodium
*
* Uses `crypto_box_easy` and `crypto_box_open_easy` for en/decryption, and `crypto_box_keypair` for key generation.
* The algorithms used are (as per https://libsodium.gitbook.io/doc/public-key_cryptography/authenticated_encryption#algorithm-details)
*
* Key exchange: X25519
* Encryption: XSalsa20
* Authentication: Poly1305
*/

class CryptoBox {
public:
    CryptoBox(util::data::byte* peerKey = nullptr);
    
    // The returned pointer lives as long as this `CryptoBox` object does.
    util::data::byte* getPublicKey();

    // The data is copied from src into a private member. You are responsible for freeing the source afterwards.
    // If the length is not equal to `crypto_box_PUBLICKEYBYTES` the behavior is undefined.
    void setPeerKey(util::data::byte* src);

    // Encrypts the data.
    util::data::bytevector encrypt(const util::data::bytevector& data);
    util::data::bytevector encrypt(const util::data::byte* data, size_t size);
    util::data::bytevector encrypt(const std::string& data);
    
    // Decrypts the data. It must include the nonce and the authentication tag.
    util::data::bytevector decrypt(const util::data::bytevector& data);
    util::data::bytevector decrypt(const util::data::byte* data, size_t size);
    size_t decryptInto(const util::data::byte* src, util::data::byte* dest, size_t size);
    size_t decryptInPlace(util::data::byte* data, size_t size);

    std::string decryptToString(const util::data::bytevector& data);
    std::string decryptToString(const util::data::byte* data, size_t size);

private: // nuh uh
    util::data::byte secretKey[crypto_box_SECRETKEYBYTES];
    util::data::byte publicKey[crypto_box_PUBLICKEYBYTES];
    util::data::byte peerPublicKey[crypto_box_PUBLICKEYBYTES];
};