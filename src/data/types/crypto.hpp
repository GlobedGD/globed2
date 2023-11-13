/*
* Connection related data structures
*/

#pragma once
#include <data/bytebuffer.hpp>
#include <util/data.hpp>
#include <crypto/box.hpp> // for crypto_box_PUBLICKEYBYTES

class CryptoPublicKey {
public:
    CryptoPublicKey() {}
    CryptoPublicKey(util::data::bytearray<crypto_box_PUBLICKEYBYTES> key) : key(key) {}

    GLOBED_ENCODE {
        buf.writeBytes(key);
    }

    GLOBED_DECODE {
        key = buf.readBytes<crypto_box_PUBLICKEYBYTES>();
    }

    util::data::bytearray<crypto_box_PUBLICKEYBYTES> key;
};

