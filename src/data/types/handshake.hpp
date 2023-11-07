/*
* Connection related data structures
*/

#pragma once
#include <data/bytebuffer.hpp>
#include <util/data.hpp>
#include <crypto/box.hpp> // for crypto_box_PUBLICKEYBYTES

class HandshakeData {
public:
    HandshakeData() {}

    GLOBED_ENCODE {
        buf.writeBytes(pubkey);
    }

    util::data::bytearray<crypto_box_PUBLICKEYBYTES> pubkey;
};

class HandshakeResponseData {
public:
    HandshakeResponseData() {}

    GLOBED_DECODE {
        serverkey = buf.readBytes<crypto_box_PUBLICKEYBYTES>();
    }

    util::data::bytearray<crypto_box_PUBLICKEYBYTES> serverkey;
};

