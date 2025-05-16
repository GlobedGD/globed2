#pragma once
#include <defs/assert.hpp>
#include <defs/minimal_geode.hpp>

#include <type_traits>
#include <fmt/format.h>
#include <asp/data/util.hpp>
#include <asp/misc/traits.hpp>

#include "basic.hpp"
#include "types/basic/either.hpp"
#include "bitbuffer.hpp"
#include "bitfield.hpp"
#include <util/data.hpp>
#include <util/misc.hpp>

class ByteBuffer {
    using length_t = uint16_t;

public:
    // Error enum.
    enum class DecodeError {
        Ok,
        NotEnoughData,
        InvalidEnumValue,
        DataTooLong,
        LengthPrefixTooLong
    };

    BOOST_DESCRIBE_NESTED_ENUM(DecodeError, Ok, NotEnoughData, InvalidEnumValue);

    template <typename T = void>
    using DecodeResult = geode::Result<T, DecodeError>;

    static const char* strerror(DecodeError err);

    // Constructs a `ByteBuffer` with no data
    ByteBuffer();

    // Construct a `ByteBuffer` with some data
    ByteBuffer(const util::data::bytevector& data);
    ByteBuffer(const util::data::byte* data, size_t length);

    // Take ownership of the given `bytevector` and construct a `ByteBuffer` from the data
    ByteBuffer(util::data::bytevector&& data);

    ByteBuffer(const ByteBuffer& other) = default;
    ByteBuffer& operator=(const ByteBuffer& other) = default;

    ByteBuffer(ByteBuffer&& other) = default;
    ByteBuffer& operator=(ByteBuffer&& other) = default;

    // Read a value from this bytebuffer
    template <typename T>
    DecodeResult<T> readValue() {
        if constexpr (util::data::IsPrimitive<T>) {
            return this->readPrimitive<T>();
        } else if constexpr (std::is_enum_v<T>) {
            return this->readEnum<T>();
        } else if constexpr (std::is_empty_v<T> && std::is_default_constructible_v<T>) {
            // zst, return a default constructed instance
            return Ok(T {});
        } else if constexpr (boost::describe::has_describe_members<T>::value) {
            // reflection stuffs
            return this->reflectionDecode<T>();
        } else {
            // finally, if everything else fails, try the common serializer
            // this will raise a linker error if `customDecode` is not specialized for T.
            return this->preCustomDecode<T>();
        }
    }

    // Write a value to this bytebuffer
    template <typename T>
    void writeValue(const T& value) {
        if constexpr (util::data::IsPrimitive<T>) {
            this->writePrimitive<T>(value);
        } else if constexpr (std::is_enum_v<T>) {
            this->writeEnum<T>(value);
        } else if constexpr (std::is_empty_v<T>) {
            // zst, do nothing
        } else if constexpr (boost::describe::has_describe_members<T>::value) {
            this->reflectionEncode<T>(value);
        } else {
            // use custom encoder
            this->preCustomEncode<T>(value);
        }
    }

    // Read a commonly encodable type. Can be specialized for any type to enable decoding ability.
    template <typename T>
    DecodeResult<T> customDecode();

    // Write a commonly encodable type. Can be specialized for any type to enable encoding ability.
    template <typename T>
    void customEncode(const T& value);

    /* Various helper methods */

    // Get the underlying data buffer of this `ByteBuffer`
    const util::data::bytevector& data() const;

    // Get the underlying data buffer of this `ByteBuffer`
    util::data::bytevector& data();

    // Clear all the data in this buffer
    void clear();

    // Get the amount of data in this `ByteBuffer`
    size_t size() const;

    // Get the current position of this buffer
    size_t getPosition() const;

    // Set the position of this buffer
    void setPosition(size_t pos);

    // Resize the internal buffer to `newSize` bytes
    void resize(size_t newSize);

    // Equivalent to `resize(size() + bytes)`
    void grow(size_t bytes);

    // Equivalent to `resize(size() - bytes)`
    void shrink(size_t bytes);

    // Skips the next `bytes` bytes. Returns an error if there aren't enough bytes to skip.
    DecodeResult<> skip(size_t bytes);

    // Returns an error if `count` bytes cannot be read from this `ByteBuffer`
    DecodeResult<> boundsCheck(size_t count);

    /* Helper methods for reading/writing a specific integer type */

    DecodeResult<bool> readBool();
    DecodeResult<uint8_t> readU8();
    DecodeResult<uint16_t> readU16();
    DecodeResult<uint32_t> readU32();
    DecodeResult<uint64_t> readU64();
    DecodeResult<int8_t> readI8();
    DecodeResult<int16_t> readI16();
    DecodeResult<int32_t> readI32();
    DecodeResult<int64_t> readI64();
    DecodeResult<float> readF32();
    DecodeResult<double> readF64();
    DecodeResult<size_t> readLength();

    // read a length `x`, return an error if unable to read at least `x` bytes from the buffer afterwards
    DecodeResult<size_t> readLengthCheck(size_t elemsize = 1);

    void writeBool(bool value);
    void writeU8(uint8_t value);
    void writeU16(uint16_t value);
    void writeU32(uint32_t value);
    void writeU64(uint64_t value);
    void writeI8(int8_t value);
    void writeI16(int16_t value);
    void writeI32(int32_t value);
    void writeI64(int64_t value);
    void writeF32(float value);
    void writeF64(double value);
    void writeLength(size_t value);

    /* Bits */
    template <size_t N>
    void writeBits(const BitBuffer<N>& bits) {
        this->writePrimitive(bits.contents());
    }

    template <size_t N>
    DecodeResult<BitBuffer<N>> readBits() {
        GLOBED_UNWRAP_INTO(this->readPrimitive<BitBufferUnderlyingType<N>>(), auto underlying);
        return Ok(BitBuffer<N>(underlying));
    }

    /* Raw reads */
    DecodeResult<> readBytesInto(util::data::byte* buf, size_t bytes);

    /* Raw write */
    void writeBytes(void* buf, size_t bytes);

protected:
    // Read `sizeof(T)` bytes and reinterpret them as `T`. No endianness conversions are done.
    template <typename T>
    DecodeResult<T> rawRead() {
        GLOBED_UNWRAP(this->boundsCheck(sizeof(T)));

        T value;
        std::memcpy(&value, _data.data() + _position, sizeof(T));
        _position += sizeof(T);

        return Ok(value);
    }

    // Write the bits of `value` into this `ByteBuffer`. No endianness conversions are done
    template <typename T>
    void rawWrite(T value) {
        const util::data::byte* bytes = reinterpret_cast<const util::data::byte*>(&value);

        this->rawWriteBytes(bytes, sizeof(T));
    }

    // Like `rawWrite` but accepts a raw buffer
    void rawWriteBytes(const util::data::byte* bytes, size_t length);

    // Read a primitive `T`, performing endianness conversions
    template <typename T>
    DecodeResult<T> readPrimitive() {
        static_assert(util::data::IsPrimitive<T>, "Type passed to readPrimitive must be a primitive");

        GLOBED_UNWRAP_INTO(this->rawRead<T>(), T value);
        value = util::data::maybeByteswap(value);

        return Ok(value);
    }

    // Write a primitive `T`, performing endianness conversions
    template <typename T>
    void writePrimitive(T value) {
        static_assert(util::data::IsPrimitive<T>, "Type passed to writePrimitive must be a primitive");

        value = util::data::maybeByteswap(value);

        this->rawWrite<T>(value);
    }

    // Read an enum
    template <typename E>
    DecodeResult<E> readEnum() {
        static_assert(std::is_enum_v<E>, "template argument passed to readEnum must be an enum");

        using P = typename std::underlying_type<E>::type;

        static_assert(util::data::IsPrimitive<P>, "enum underlying type must be a primitive");

        GLOBED_UNWRAP_INTO(this->readPrimitive<P>(), P underlying);

        // validate the enum - if there's no descriptor matching the decoded value, raise an error

        bool foundMatch = false;

        boost::mp11::mp_for_each<boost::describe::describe_enumerators<E>>([&](auto descriptor) {
            E val = descriptor.value;
            P valUnderlying = static_cast<P>(val);

            if (valUnderlying == underlying) {
                foundMatch = true;
            }
        });

        if (!foundMatch) {
            return Err(DecodeError::InvalidEnumValue);
        }

        return Ok(static_cast<E>(underlying));
    }

    // Write an enum
    template <typename E>
    void writeEnum(E value) {
        static_assert(std::is_enum_v<E>, "template argument passed to writeEnum must be an enum");

        using P = typename std::underlying_type<E>::type;

        static_assert(util::data::IsPrimitive<P>, "enum underlying type must be a primitive");

        this->writePrimitive<P>(static_cast<P>(value));
    }

    // Read a value using boost reflection
    template <
        typename T,
        class Md = boost::describe::describe_members<T, boost::describe::mod_public>,
        class Bd = boost::describe::describe_bases<T, boost::describe::mod_any_access>
    >
    requires std::is_default_constructible_v<T>
    DecodeResult<T> reflectionDecode() {
        static_assert(std::is_class_v<T>, "attempted to call reflectionDecode on a non-class type");

        // if it's a bitfield, decode it as such
        if constexpr (!boost::mp11::mp_empty<Bd>::value) {
            if constexpr (std::is_same_v<typename boost::mp11::mp_first<Bd>::type, BitfieldBase>) {
                return reflectionDecodeBitfield<T>();
            }
        } else {
            checkMissingFields<T>();
        }

        // create a default initialized instance
        T value;

        bool failed = false;
        DecodeError failError;

        boost::mp11::mp_for_each<Md>([&, this](auto descriptor) -> void {
            if (failed) return;

            // terrifying
            using MPT = decltype(descriptor.pointer);
            using FT = typename asp::member_ptr_to_underlying<MPT>::type;

            auto result = this->readValue<FT>();
            if (result.isErr()) {
                failed = true;
                failError = result.unwrapErr();
            } else {
                value.*descriptor.pointer = std::move(result.unwrap());
            }
        });

        if (failed) {
            return Err(std::move(failError));
        }

        return Ok(std::move(value));
    }

    // Write a value using boost reflection
    template <
        typename T,
        class Md = boost::describe::describe_members<T, boost::describe::mod_public>,
        class Bd = boost::describe::describe_bases<T, boost::describe::mod_any_access>
    >
    void reflectionEncode(const T& value) {
        static_assert(std::is_class_v<T>, "attempted to call reflectionEncode on a non-class type");

        // if it's a bitfield struct, encode it as such
        if constexpr (!boost::mp11::mp_empty<Bd>::value) {
            if constexpr (std::is_same_v<typename boost::mp11::mp_first<Bd>::type, BitfieldBase>) {
                this->reflectionEncodeBitfield<T>(value);
                return;
            }
        } else {
            checkMissingFields<T>();
        }

        boost::mp11::mp_for_each<Md>([&, this](auto descriptor) {
            this->writeValue(value.*descriptor.pointer);
        });
    }

    template <
        typename T,
        class Md = boost::describe::describe_members<T, boost::describe::mod_public>
    >
    DecodeResult<T> reflectionDecodeBitfield() {
        static_assert(sizeof(T) <= 64, "unable to decode a bitfield with over 64 fields");

        constexpr size_t bitcount = util::data::bitsToBytes(sizeof(T)) * 8;
        GLOBED_UNWRAP_INTO(this->readBits<bitcount>(), auto bits);

        T value;
        boost::mp11::mp_for_each<Md>([&, this](auto descriptor) -> void {
            using MPT = decltype(descriptor.pointer);
            using FT = typename asp::member_ptr_to_underlying<MPT>::type;

            static_assert(std::is_same_v<FT, bool>, "bitfield struct must consist of only bools");

            value.*descriptor.pointer = bits.readBit();
        });

        return Ok(std::move(value));
    }

    template <
        typename T,
        class Md = boost::describe::describe_members<T, boost::describe::mod_public>
    >
    void reflectionEncodeBitfield(const T& value) {
        static_assert(sizeof(T) <= 64, "unable to encode a bitfield with over 64 fields");

        // so evil
        constexpr size_t bitcount = util::data::bitsToBytes(sizeof(T)) * 8;
        BitBuffer<bitcount> bits;

        boost::mp11::mp_for_each<Md>([&, this](auto descriptor) -> void {
            using MPT = decltype(descriptor.pointer);
            using FT = typename asp::member_ptr_to_underlying<MPT>::type;

            static_assert(std::is_same_v<FT, bool>, "bitfield struct must consist of only bools");

            bits.writeBit(value.*descriptor.pointer);
        });

        this->writeBits(bits);
    }

    /* Some templated specializations */

    template <typename T>
    DecodeResult<T> preCustomDecode() {
        if constexpr (asp::is_std_vector<T>::value) {
            return this->pcDecodeVector<typename T::value_type>();
        } else if constexpr (asp::is_std_pair<T>::value) {
            return this->pcDecodePair<typename T::first_type, typename T::second_type>();
        } else if constexpr (asp::is_std_optional<T>::value) {
            return this->pcDecodeOptional<typename T::value_type>();
        } else if constexpr (util::misc::is_map<T>::value) {
            return this->pcDecodeMap<typename T::key_type, typename T::mapped_type>();
        } else if constexpr (util::misc::is_either<T>::value) {
            return this->pcDecodeEither<typename T::first_type, typename T::second_type>();
        } else {
            return this->customDecode<T>();
        }
    }

    template <typename T>
    void preCustomEncode(const T& value) {
        if constexpr (asp::is_std_vector<T>::value) {
            this->pcEncodeVector<typename T::value_type>(value);
        } else if constexpr (asp::is_std_pair<T>::value) {
            this->pcEncodePair<typename T::first_type, typename T::second_type>(value);
        } else if constexpr (asp::is_std_optional<T>::value) {
            this->pcEncodeOptional<typename T::value_type>(value);
        } else if constexpr (util::misc::is_either<T>::value) {
            this->pcEncodeEither(value);
        } else if constexpr (util::misc::is_map<T>::value) {
            this->pcEncodeMap<typename T::key_type, typename T::mapped_type>(value);
        } else if constexpr (std::is_same_v<T, ByteBuffer>) {
            this->rawWriteBytes(value.data().data(), value.size());
        } else {
            this->customEncode(value);
        }
    }

    // Vector

    template<typename T>
    DecodeResult<std::vector<T>> pcDecodeVector() {
        GLOBED_UNWRAP_INTO(this->readLength(), auto length);

        std::vector<T> out;

        if (sizeof(T) * length < (2 << 15)) {
            out.reserve(length);
        }

        for (size_t i = 0; i < length; i++) {
            GLOBED_UNWRAP_INTO(this->readValue<T>(), T val);
            out.emplace_back(std::move(val));
        }

        return Ok(std::move(out));
    }

    template<typename T>
    void pcEncodeVector(const std::vector<T>& vec) {
        this->writeLength(vec.size());

        for (const auto& elem : vec) {
            this->writeValue<T>(elem);
        }
    }

    // Map

    template <typename T, typename Y>
    DecodeResult<std::map<T, Y>> pcDecodeMap() {
        GLOBED_UNWRAP_INTO(this->readLength(), auto length);

        std::map<T, Y> out;

        for (size_t i = 0; i < length; i++) {
            GLOBED_UNWRAP_INTO(this->readValue<T>(), T first);
            GLOBED_UNWRAP_INTO(this->readValue<Y>(), Y second);

            out.emplace(std::make_pair(std::move(first), std::move(second)));
        }

        return Ok(std::move(out));
    }

    template <typename T, typename Y>
    void pcEncodeMap(const std::map<T, Y>& map) {
        this->writeLength(map.size());

        for (const auto& [first, second] : map) {
            this->writeValue<T>(first);
            this->writeValue<Y>(second);
        }
    }

    // Pair

    template <typename T1, typename T2>
    DecodeResult<std::pair<T1, T2>> pcDecodePair() {
        GLOBED_UNWRAP_INTO(this->readValue<T1>(), auto first);
        GLOBED_UNWRAP_INTO(this->readValue<T2>(), auto second);

        return Ok(std::move(std::make_pair(std::move(first), std::move(second))));
    }

    template <typename T1, typename T2>
    void pcEncodePair(const std::pair<T1, T2>& pair) {
        this->writeValue<T1>(pair.first);
        this->writeValue<T2>(pair.second);
    }

    // Optional

    template <typename T>
    DecodeResult<std::optional<T>> pcDecodeOptional() {
        GLOBED_UNWRAP_INTO(this->readBool(), bool present);

        if (!present) {
            return Ok(std::nullopt);
        }

        GLOBED_UNWRAP_INTO(this->readValue<T>(), auto value);

        return Ok(std::move(value));
    }

    template <typename T>
    void pcEncodeOptional(const std::optional<T>& opt) {
        this->writeBool(opt.has_value());

        if (opt.has_value()) {
            this->writeValue<T>(opt.value());
        }
    }

    // Either

    template <typename T, typename Y>
    DecodeResult<Either<T, Y>> pcDecodeEither() {
        GLOBED_UNWRAP_INTO(this->readBool(), bool isFirst);

        if (isFirst) {
            GLOBED_UNWRAP_INTO(this->readValue<T>(), T value);
            return Ok(std::move(Either<T, Y>(value)));
        } else {
            GLOBED_UNWRAP_INTO(this->readValue<Y>(), Y value);
            return Ok(std::move(Either<T, Y>(value)));
        }
    }

    template <typename T, typename Y>
    void pcEncodeEither(const Either<T, Y>& either) {
        this->writeBool(either.isFirst());

        if (either.isFirst()) {
            this->writeValue<T>(either.firstRef()->get());
        } else {
            this->writeValue<Y>(either.secondRef()->get());
        }
    }

public:
    template <
        typename T,
        class Md = boost::describe::describe_members<T, boost::describe::mod_public>
    >
    constexpr static size_t calculateStructSize() {
        size_t total = 0;
        size_t structAlignment = 1;

        // this is bad bad bad, but we assume there can only be 1 vtable.
        if constexpr (std::is_polymorphic_v<T>) {
            structAlignment = alignof(void*);
            total += sizeof(void*);
        }

        boost::mp11::mp_for_each<Md>([&](auto descriptor) {
            using MPT = decltype(descriptor.pointer);
            using FT = typename asp::member_ptr_to_underlying<MPT>::type;

            // align first if unaligned
            if (total % alignof(FT) != 0) {
                total = total + (alignof(FT) - total % alignof(FT));
            }

            total += sizeof(FT);

            // struct alignment is based on the member with the largest alignment
            if (alignof(FT) > structAlignment) {
                structAlignment = alignof(FT);
            }
        });

        // align struct
        if (total % structAlignment != 0) {
            total = total + (structAlignment - total % structAlignment);
        }

        return total;
    }

    template <typename T>
    constexpr static void checkMissingFields() {
        static_assert(calculateStructSize<T>() == sizeof(T), "size of the type does not match the sizes of all fields, make sure fields are listed in the correct order and there are no missing fields");
    }

private:
    // Data members
    util::data::bytevector _data;
    size_t _position = 0;
};

// Custom error formatter
template <>
struct fmt::formatter<ByteBuffer::DecodeError> {
private:
    using U = std::underlying_type_t<ByteBuffer::DecodeError>;
    fmt::formatter<fmt::string_view, char> sf_;
    fmt::formatter<U, char> nf_;

public:
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(ByteBuffer::DecodeError error, format_context& ctx) const {
        auto* s = boost::describe::enum_to_string(error, 0);

        if (s) {
            return sf_.format(s, ctx);
        } else {
            return nf_.format(static_cast<U>(error), ctx);
        }
    }
};
