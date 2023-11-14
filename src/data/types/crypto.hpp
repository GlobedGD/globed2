/*
* Connection related data structures
*/

#pragma once
#include <data/bytebuffer.hpp>
#include <util/data.hpp>
#include <crypto/box.hpp>

class CryptoPublicKey {
public:
    CryptoPublicKey() {}
    CryptoPublicKey(util::data::bytearray<CryptoBox::KEY_LEN> key) : key(key) {}

    GLOBED_ENCODE {
        buf.writeBytes(key);
    }

    GLOBED_DECODE {
        key = buf.readBytes<CryptoBox::KEY_LEN>();
    }

    util::data::bytearray<CryptoBox::KEY_LEN> key;
};

