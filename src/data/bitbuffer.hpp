#pragma once
#include <defs/assert.hpp>
#include <defs/minimal_geode.hpp>

#include <bitset>
#include <vector>

/*
* BitBuffer - a simple interface that allows you to read/write bits (MSB to LSB)
*/
template <size_t BitCount> requires (BitCount <= 64 && BitCount > 0)
class BitBuffer {
public:
    using UnderlyingType = std::conditional_t<(BitCount <= 8), uint8_t,
                 std::conditional_t<(BitCount <= 16), uint16_t,
                 std::conditional_t<(BitCount <= 32), uint32_t,
                 std::conditional_t<(BitCount <= 64), uint64_t, void>>>>;

    explicit BitBuffer(UnderlyingType val) {
        bitset = std::bitset<BitCount>(val);
    }

    BitBuffer() {}

    template <typename... Args> requires (std::same_as<Args, bool> && ...)
    BitBuffer(Args... args) {
        this->writeBits(args...);
    }

    void writeBit(bool value) {
        // remember - we are writing from MSB to LSB so we need to reverse the position
        GLOBED_REQUIRE(position != BitCount, "BitBuffer tried to write too many bits");
        size_t actualPosition = BitCount - ++position;

        bitset.set(actualPosition, value);
    }

    template <typename... Args> requires (std::same_as<Args, bool> && ...)
    void writeBits(Args... args) {
        (writeBit(args), ...);
    }

    void writeBits(const std::vector<bool>& values) {
        for (bool value : values) {
            writeBit(value);
        }
    }

    bool readBit() {
        GLOBED_REQUIRE(position != BitCount, "BitBuffer tried to read too many bits");
        size_t actualPosition = BitCount - ++position;

        return bitset.test(actualPosition);
    }

    void readBitInto(bool& out) {
        out = readBit();
    }

    template <typename... Args> requires (std::same_as<Args, bool> && ...)
    void readBitsInto(Args&... args) {
        (readBitInto(args), ...);
    }

    UnderlyingType contents() const {
        return static_cast<UnderlyingType>(bitset.to_ullong());
    }

    size_t getPosition() {
        return position;
    }

    void setPosition(size_t pos) {
        position = pos;
    }

    void clear() {
        bitset.reset();
    }

    constexpr size_t size() {
        return BitCount;
    }
private:
    std::bitset<BitCount> bitset;
    size_t position = 0;
};

template <size_t BitCount>
using BitBufferUnderlyingType = typename BitBuffer<BitCount>::UnderlyingType;