#include "room.hpp"

template<> void ByteBuffer::customEncode(const RoomSettings& data) {
    BitBuffer<16> bits;
    bits.writeBits(
        data.collision
    );
    this->writeBits(bits);
    this->writeU64(data.reserved);
}

template<> ByteBuffer::DecodeResult<RoomSettings> ByteBuffer::customDecode() {
    RoomSettings data;

    GLOBED_UNWRAP_INTO(this->readBits<16>(), auto bits);
    bits.readBitsInto(
        data.collision
    );

    GLOBED_UNWRAP_INTO(this->readU64(), data.reserved);

    return Ok(data);
}
