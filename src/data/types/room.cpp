#include "room.hpp"

template<> void ByteBuffer::customEncode(const RoomSettings& data) {
    BitBuffer<16> bits;
    bits.writeBits(
        data.inviteOnly,
        data.publicInvites,
        data.collision,
        data.twoPlayerMode
    );
    this->writeBits(bits);
    this->writeU64(data.reserved);
}

template<> ByteBuffer::DecodeResult<RoomSettings> ByteBuffer::customDecode() {
    RoomSettings data;

    GLOBED_UNWRAP_INTO(this->readBits<16>(), auto bits);
    bits.readBitsInto(
        data.inviteOnly,
        data.publicInvites,
        data.collision,
        data.twoPlayerMode
    );

    GLOBED_UNWRAP_INTO(this->readU64(), data.reserved);

    return Ok(data);
}
