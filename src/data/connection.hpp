/*
* Connection related data structures
*/

#pragma once
#include "bytebuffer.hpp"
#include <util/data.hpp>
#include <crypto/box.hpp> // for crypto_box_PUBLICKEYBYTES

class HandshakeData {
public:
    HandshakeData();

    void encode(ByteBuffer& buf) {
        buf.writeBytes(pubkey);
    }

    void decode(ByteBuffer& buf) {
        GLOBED_UNIMPL("HandshakeData::decode unimplemented")
    }

    util::data::bytevector pubkey;
};

class HandshakeResponseData {
public:
    HandshakeResponseData();

    void encode(ByteBuffer& buf) {
        GLOBED_UNIMPL("HandshakeResponseData::encode unimplemented")
    }

    void decode(ByteBuffer& buf) {
        serverkey = buf.readBytes(crypto_box_PUBLICKEYBYTES);
    }

    util::data::bytevector serverkey;
};

