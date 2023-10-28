#pragma once
#include "bitbuffer.hpp"
#include <defs.hpp>
#include <util/data.hpp>

class ByteBuffer;
template <typename T>
concept Serializable = requires(T t, ByteBuffer& buf) {
    { t.encode(buf) } -> std::same_as<void>;
    { t.decode(buf) } -> std::same_as<void>;
    T();
};

class ByteBuffer {
public:
    // Constructs an empty ByteBuffer
    ByteBuffer();

    // Construct a ByteBuffer and initializes it with some data
    ByteBuffer(const util::data::bytevector& data);
    ByteBuffer(const util::data::byte* data, size_t length);

    template<typename T>
    T read();

    template<typename T>
    void write(T value);

    // Read and write method for primitive types

    uint8_t readU8();
    int8_t readI8();
    uint16_t readU16();
    int16_t readI16();
    uint32_t readU32();
    int32_t readI32();
    uint64_t readU64();
    int64_t readI64();
    float readF32();
    double readF64();

    void writeU8(uint8_t value);
    void writeI8(int8_t value);
    void writeU16(uint16_t value);
    void writeI16(int16_t value);
    void writeU32(uint32_t value);
    void writeI32(int32_t value);
    void writeU64(uint64_t value);
    void writeI64(int64_t value);
    void writeF32(float value);
    void writeF64(double value);

    // Read and write methods for dynamic-sized types
    std::string readString();
    util::data::bytevector readByteArray();

    void writeString(const std::string& str);
    void writeByteArray(const util::data::bytevector& vec);

    // readBytes/writeBytes doesn't prepend the size unlike writeByteArray
    util::data::bytevector readBytes(size_t size);
    void writeBytes(const util::data::byte* data, size_t size);
    void writeBytes(const util::data::bytevector& vec);

    // Read and write methods for bit manipulation

    template<size_t BitCount>
    BitBuffer<BitCount> readBits() {
        constexpr size_t byteCount = util::data::bitsToBytes(BitCount);
        boundsCheck(byteCount);

        auto value = read<BitBufferUnderlyingType<BitCount>>();
        return BitBuffer<BitCount>(value);
    }

    template<size_t BitCount>
    void writeBits(BitBuffer<BitCount> bitbuf) {
        constexpr size_t byteCount = util::data::bitsToBytes(BitCount);

        write(bitbuf.contents());
    }

    // Read and write methods for types implementing Serializable

    template <Serializable T>
    T readValue() {
        T value;
        value.decode(*this);
        return value;
    }

    template <Serializable T>
    void writeValue(T value) {
        value.encode(*this);
    }

    // Get the internal data as a bytevector
    util::data::bytevector getData() const;

    // Get a reference to internal data instead of a copy
    util::data::bytevector& getDataRef();

    size_t size() const;
    void clear();

    size_t getPosition() const;
    void setPosition(size_t pos);

    // Resize the internal vector to length `bytes`. Does not change the position
    void resize(size_t bytes);

    // Grow the internal vector, synonym to `resize(_data.len() + bytes)`
    void grow(size_t bytes);

    // Shrink the internal vector, synonym to `resize(_data.len() - bytes)`
    void shrink(size_t bytes);
private:
    util::data::bytevector _data;
    size_t _position;

    inline void boundsCheck(size_t readBytes) {
        GLOBED_ASSERT(_position + readBytes <= _data.size(), "ByteBuffer out of bounds read")
    }
};