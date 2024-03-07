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

    util::data::bytearray<CryptoBox::KEY_LEN> key;
};

GLOBED_SERIALIZABLE_STRUCT(CryptoPublicKey, (key));
