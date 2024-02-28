#pragma once
#include <type_traits>

#include "bitbuffer.hpp"
#include <util/data.hpp>

class ByteBuffer;

// Represents a data type that can be easily written to a ByteBuffer
template <typename T>
concept Encodable = requires(const T t, ByteBuffer& buf) {
    { t.encode(buf) } -> std::same_as<void>;
};

// Represents a data type that can be easily read from a ByteBuffer
template <typename T>
concept Decodable = requires(T t, ByteBuffer& buf) {
    { t.decode(buf) } -> std::same_as<void>;
} && std::is_default_constructible_v<T>;

// Represents a data type that can be easily read or written to/from a ByteBuffer
template <typename T>
concept Serializable = Encodable<T> && Decodable<T>;

// helper macros so you can do GLOBED_ENCODE {...} in serializable structs or packets
#define GLOBED_ENCODE inline void encode(ByteBuffer& buf) const
#define GLOBED_DECODE inline void decode(ByteBuffer& buf)

class ByteBuffer {
public:
    // Constructs an empty ByteBuffer
    ByteBuffer();

    // Construct a ByteBuffer and initializes it with some data
    ByteBuffer(const util::data::bytevector& data);
    ByteBuffer(const util::data::byte* data, size_t length);

    ByteBuffer(util::data::bytevector&& data);

    // Read a primitive type T. It is highly advised not to use this directly, as it performs no endianness conversions.
    template<typename T>
    T read();

    // Write a primitive type T. It is highly advised not to use this directly, as it performs no endianness conversions.
    template<typename T>
    void write(T value);

    /*
    * Read and write method for primitive types
    */

    bool readBool();
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

    void writeBool(bool value);
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

    /*
    * Read and write methods for dynamic-sized types
    */

    // keep in mind strings might be broken if non ascii characters are sent

    // Read a string, prefixed with 4 bytes indicating length
    std::string readString();
    // Read a bytevector, prefixed with 4 bytes indicating length
    util::data::bytevector readByteArray();

    // Write a string, prefixed with 4 bytes indicating length
    void writeString(const std::string_view str);
    // Write a bytevector, prefixed with 4 bytes indicating length
    void writeByteArray(const util::data::bytevector& vec);
    // Write bytes from a buffer, prefixed with 4 bytes indicating length
    void writeByteArray(const util::data::byte* data, size_t length);

    /*
    * Read and write methods for fixed-size types
    */

    // Read a fixed-size bytevector
    util::data::bytevector readBytes(size_t size);

    // Read a bytearray whose size is known at compile time (without length prefix)
    template <size_t Count>
    util::data::bytearray<Count> readBytes() {
        this->boundsCheck(Count);
        util::data::bytearray<Count> arr;
        std::copy(_data.begin() + _position, _data.begin() + _position + Count, arr.begin());

        _position += Count;

        return arr;
    }

    // Read `size` bytes into the pointer `out`
    void readBytesInto(util::data::byte* out, size_t size);

    // Write a fixed-size buffer of bytes without length prefix. If the size isn't a constant,
    // it is recommended to use writeByteArray instead.
    void writeBytes(const util::data::byte* data, size_t size);
    // Write a fixed-size bytevector without length prefix. If the size isn't a constant,
    // it is recommended to use writeByteArray instead.
    void writeBytes(const util::data::bytevector& vec);
    // Write a bytearray whose size is known at compile time (without length prefix)
    template <size_t Count>
    void writeBytes(const util::data::bytearray<Count>& arr) {
        this->writeBytes(arr.data(), Count);
    }

    /*
    * Read and write methods for bit manipulation
    */

    // Read `BitCount` bits from this ByteBuffer. The bit amount must be known at compile time.
    // The amount of bytes read is rounded up from the bit count. So reading 10 bits would read 2 whole bytes.
    template<size_t BitCount>
    BitBuffer<BitCount> readBits() {
        constexpr size_t byteCount = util::data::bitsToBytes(BitCount);
        this->boundsCheck(byteCount);

        auto value = this->read<BitBufferUnderlyingType<BitCount>>();

        return BitBuffer<BitCount>(value);
    }

    // Write all bits from the given `BitBuffer` into the current `ByteBuffer`
    template<size_t BitCount>
    void writeBits(BitBuffer<BitCount> bitbuf) {
        this->write(bitbuf.contents());
    }

    /*
    * Read and write methods for types implementing Encodable/Decodable/Serializable
    */

    // Read a `Decodable` object
    template <Decodable T>
    T readValue() {
        T value;
        value.decode(*this);
        return value;
    }

    // Write an `Encodable` object
    template <Encodable T>
    void writeValue(const T& value) {
        value.encode(*this);
    }

    // Read an `std::optional<T>` where T: Decodable
    template <Decodable T>
    std::optional<T> readOptionalValue() {
        std::optional<T> value;
        if (this->readBool()) {
            value = this->readValue<T>();
        }

        return value;
    }

    template <Encodable T>
    void writeOptionalValue(const std::optional<T>& value) {
        this->writeBool(value.has_value());
        if (value) {
            this->writeValue(value.value());
        }
    }

    template <typename T>
    void writeOptionalValue(const std::optional<T>& value, std::function<void(ByteBuffer&, const T& val)> encodeFunc) {
        this->writeBool(value.has_value());
        if (value) {
            encodeFunc(*this, value.value());
        }
    }

    // Read a list of `Decodable` objects, prefixed with 4 bytes indicating the count.
    template <Decodable T>
    std::vector<T> readValueVector() {
        std::vector<T> out;
        this->readValueVectorInto(out);
        return out;
    }

    // Same as `readValueVector` but you get to specify the destination
    template <Decodable T>
    void readValueVectorInto(std::vector<T>& out) {
        auto length = this->readU32();

        for (size_t i = 0; i < length; i++) {
            out.push_back(this->readValue<T>());
        }
    }

    // Read an array of `Decodable` objects, the count must be known at compile time.
    template <Decodable T, size_t Count>
    std::array<T, Count> readValueArray() {
        std::array<T, Count> out;
        for (size_t i = 0; i < Count; i++) {
            out[i] = this->readValue<T>();
        }

        return out;
    }

    // Write a list of `Encodable` objects, prefixed with 4 bytes indicating the count.
    template <Encodable T>
    void writeValueVector(const std::vector<T>& values) {
        this->writeU32(values.size());
        for (const T& value : values) {
            value.encode(*this);
        }
    }

    // Write an array of `Encodable` objects, without encoding the size.
    template <Encodable T, size_t Count>
    void writeValueArray(const std::array<T, Count>& values) {
        for (const T& value : values) {
            value.encode(*this);
        }
    }

    // Read a generic primitive number (like read<T> but with endianness checks)
    template <typename T>
    T readPrimitive() {
        static_assert(util::data::IsPrimitive<T>, "Unsupported type for readPrimitive, must be a primitive");

        if constexpr (std::is_same_v<T, uint16_t>) {
            return readU16();
        } else if constexpr (std::is_same_v<T, uint32_t>) {
            return readU32();
        } else if constexpr (std::is_same_v<T, uint64_t>) {
            return readU64();
        } else if constexpr (std::is_same_v<T, float>) {
            return readF32();
        } else if constexpr (std::is_same_v<T, double>) {
            return readF64();
        } else if constexpr (std::is_same_v<T, int16_t>) {
            return readI16();
        } else if constexpr (std::is_same_v<T, int32_t>) {
            return readI32();
        } else if constexpr (std::is_same_v<T, int64_t>) {
            return readI64();
        } else if constexpr (std::is_same_v<T, uint8_t>) {
            return readU8();
        } else if constexpr(std::is_same_v<T, int8_t>) {
            return readI8();
        }
    }

    // Write a generic primitive number (like read<T> but with endianness checks)
    template <typename T>
    void writePrimitive(T value) {
        static_assert(util::data::IsPrimitive<T>, "Unsupported type for writePrimitive, must be a primitive");

        if constexpr (std::is_same_v<T, uint16_t>) {
            writeU16(value);
        } else if constexpr (std::is_same_v<T, uint32_t>) {
            writeU32(value);
        } else if constexpr (std::is_same_v<T, uint64_t>) {
            writeU64(value);
        } else if constexpr (std::is_same_v<T, float>) {
            writeF32(value);
        } else if constexpr (std::is_same_v<T, double>) {
            writeF64(value);
        } else if constexpr (std::is_same_v<T, int16_t>) {
            writeI16(value);
        } else if constexpr (std::is_same_v<T, int32_t>) {
            writeI32(value);
        } else if constexpr (std::is_same_v<T, int64_t>) {
            writeI64(value);
        } else if constexpr (std::is_same_v<T, uint8_t>) {
            writeU8(value);
        } else if constexpr(std::is_same_v<T, int8_t>) {
            writeI8(value);
        }
    }

    // Write an enum
    template <typename E>
    void writeEnum(const E& val) {
        // macos clang is so fire
        using P = typename std::underlying_type<E>::type;
        static_assert(util::data::IsPrimitive<P>, "enum underlying type must be a primitive");

        this->writePrimitive<P>(static_cast<P>(val));
    }

    // Read an enum
    // TODO no validation, likely not planning to add but still important to know
    template <typename E>
    E readEnum() {
        using P = typename std::underlying_type<E>::type;
        static_assert(util::data::IsPrimitive<P>, "enum underlying type must be a primitive");

        return static_cast<E>(this->readPrimitive<P>());
    }

    /*
    * Cocos/GD serializable methods
    */

    // Read an RGB color (3 bytes)
    cocos2d::ccColor3B readColor3();
    // Read an RGBA color (4 bytes)
    cocos2d::ccColor4B readColor4();
    // Read a CCPoint (2 floats)
    cocos2d::CCPoint readPoint();

    // Write an RGB color (3 bytes)
    void writeColor3(cocos2d::ccColor3B color);
    // Write an RGBA color (4 bytes)
    void writeColor4(cocos2d::ccColor4B color);
    // Write a CCPoint (2 floats)
    void writePoint(cocos2d::CCPoint point);

    /*
    * Misc util functions
    */

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

    void boundsCheck(size_t readBytes);
};