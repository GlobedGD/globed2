/*
* Connection related data structures
*/

#pragma once
#include "bytebuffer.hpp"
#include <util/data.hpp>
#include <crypto/box.hpp> // for crypto_box_PUBLICKEYBYTES

class HandshakeData {
public:
    HandshakeData() {}

    void encode(ByteBuffer& buf) const {
        buf.writeBytes(pubkey);
    }

    util::data::bytearray<crypto_box_PUBLICKEYBYTES> pubkey;
};

class HandshakeResponseData {
public:
    HandshakeResponseData() {}

    void decode(ByteBuffer& buf) {
        serverkey = buf.readBytes<crypto_box_PUBLICKEYBYTES>();
    }

    util::data::bytearray<crypto_box_PUBLICKEYBYTES> serverkey;
};

