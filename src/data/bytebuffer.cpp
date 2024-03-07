#include "bytebuffer.hpp"

#include <boost/describe.hpp>

template <typename T = std::monostate>
using DecodeResult = ByteBuffer::DecodeResult<T>;
using DecodeError = ByteBuffer::DecodeError;

using namespace util::data;
using namespace cocos2d;

const char* ByteBuffer::strerror(DecodeError err) {
    using Error = ByteBuffer::DecodeError;

    switch (err) {
        case Error::Ok: return "No error";
        case Error::NotEnoughData: return "Could not read enough bytes from the buffer";
        case Error::InvalidEnumValue: return "Invalid enum value was read";
        case Error::DataTooLong: return "Received data is too long so packet decoding was halted";
    }

    return "Unknown error";
}

ByteBuffer::ByteBuffer() {}

ByteBuffer::ByteBuffer(const bytevector& data) : _data(data) {}
ByteBuffer::ByteBuffer(const byte* data, size_t length)
    : _data(bytevector(data, data + length)) {}

ByteBuffer::ByteBuffer(bytevector&& data)
    : _data(std::move(data)) {}

void ByteBuffer::rawWriteBytes(const byte* bytes, size_t length) {
    // if we can't fit (i.e. writing at the end, just use insert)
    if (_position + length > _data.size()) {
        _data.insert(_data.begin() + _position, bytes, bytes + length);
        // remove leftover elements
        _data.reserve(_position + length);
    } else {
        // otherwise, overwrite existing elements
        for (size_t i = 0; i < length; i++) {
            _data.data()[_position + i] = bytes[i];
        }
    }

    _position += length;
}

DecodeResult<> ByteBuffer::boundsCheck(size_t count) {
    if (_position + count > _data.size()) {
        return Err(DecodeError::NotEnoughData);
    }

    return Ok();
}

/* Util methods */

const bytevector& ByteBuffer::data() const {
    return _data;
}

bytevector& ByteBuffer::data() {
    return _data;
}

void ByteBuffer::clear() {
    _data.clear();
    _position = 0;
}

size_t ByteBuffer::size() const {
    return _data.size();
}

size_t ByteBuffer::getPosition() const {
    return _position;
};

void ByteBuffer::setPosition(size_t pos) {
    _position = pos;
}

void ByteBuffer::resize(size_t newSize) {
    _data.resize(newSize);
}

void ByteBuffer::grow(size_t bytes) {
    this->resize(this->size() + bytes);
}

void ByteBuffer::shrink(size_t bytes) {
    this->resize(this->size() - bytes);
}

DecodeResult<> ByteBuffer::skip(size_t bytes) {
    GLOBED_UNWRAP(this->boundsCheck(bytes));
    _position += bytes;

    return Ok();
}

DecodeResult<> ByteBuffer::readBytesInto(byte* buf, size_t bytes) {
    GLOBED_UNWRAP(this->boundsCheck(bytes));
    std::memcpy(buf, _data.data() + _position, bytes);
    _position += bytes;

    return Ok();
}

/* Common encode/decode specializations */

// Strings

template<> void ByteBuffer::customEncode(const std::string_view& value) {
    this->writePrimitive<uint32_t>(value.size());
    this->rawWriteBytes(reinterpret_cast<const byte*>(value.data()), value.size());
}

template<> void ByteBuffer::customEncode(const std::string& value) {
    this->customEncode(std::string_view(value));
}

template<> DecodeResult<std::string> ByteBuffer::customDecode() {
    GLOBED_UNWRAP_INTO(this->readPrimitive<uint32_t>(), size_t length);

    GLOBED_UNWRAP(this->boundsCheck(length));

    std::string str(reinterpret_cast<const char*>(_data.data() + _position), length);
    _position += length;

    return Ok(str);
}

// CCPoint

template<> void ByteBuffer::customEncode(const CCPoint& point) {
    this->writeF32(point.x);
    this->writeF32(point.y);
}

template<> DecodeResult<CCPoint> ByteBuffer::customDecode() {
    GLOBED_UNWRAP_INTO(this->readF32(), float x);
    GLOBED_UNWRAP_INTO(this->readF32(), float y);

    return Ok(CCPoint { x, y });
}

// CCSize

template<> void ByteBuffer::customEncode(const CCSize& size) {
    this->writeF32(size.width);
    this->writeF32(size.height);
}

template<> DecodeResult<CCSize> ByteBuffer::customDecode() {
    GLOBED_UNWRAP_INTO(this->readF32(), float w);
    GLOBED_UNWRAP_INTO(this->readF32(), float h);

    return Ok(CCSize { w, h });
}

// ccColor3B

template<> void ByteBuffer::customEncode(const ccColor3B& color) {
    this->writeU8(color.r);
    this->writeU8(color.g);
    this->writeU8(color.b);
}

template<> DecodeResult<ccColor3B> ByteBuffer::customDecode() {
    GLOBED_UNWRAP_INTO(this->readU8(), uint8_t r);
    GLOBED_UNWRAP_INTO(this->readU8(), uint8_t g);
    GLOBED_UNWRAP_INTO(this->readU8(), uint8_t b);

    return Ok(ccc3(r, g, b));
}

// ccColor4B

template<> void ByteBuffer::customEncode(const ccColor4B& color) {
    this->writeU8(color.r);
    this->writeU8(color.g);
    this->writeU8(color.b);
    this->writeU8(color.a);
}

template<> DecodeResult<ccColor4B> ByteBuffer::customDecode() {
    GLOBED_UNWRAP_INTO(this->readU8(), uint8_t r);
    GLOBED_UNWRAP_INTO(this->readU8(), uint8_t g);
    GLOBED_UNWRAP_INTO(this->readU8(), uint8_t b);
    GLOBED_UNWRAP_INTO(this->readU8(), uint8_t a);

    return Ok(ccc4(r, g, b, a));
}

// bytearray<10>
#define MAKE_ARRAY_FUNCS(sz) \
    template<> void ByteBuffer::customEncode(const bytearray<sz>& data) { \
        this->rawWriteBytes(data.data(), sz); \
    } \
    \
    template<> DecodeResult<bytearray<sz>> ByteBuffer::customDecode() { \
        bytearray<sz> out; \
        for (size_t i = 0; i < sz; i++) { \
            GLOBED_UNWRAP_INTO(this->readU8(), out[i]); \
        } \
        return Ok(out); \
    }

MAKE_ARRAY_FUNCS(10)
MAKE_ARRAY_FUNCS(32)

/* Boring ass methods */

#define MAKE_METHOD(type, name) \
    DecodeResult<type> ByteBuffer::read##name() { return this->readPrimitive<type>(); } \
    void ByteBuffer::write##name(type value) { this->writePrimitive<type>(value); }

MAKE_METHOD(bool, Bool);
MAKE_METHOD(int8_t, I8);
MAKE_METHOD(int16_t, I16);
MAKE_METHOD(int32_t, I32);
MAKE_METHOD(int64_t, I64);
MAKE_METHOD(uint8_t, U8);
MAKE_METHOD(uint16_t, U16);
MAKE_METHOD(uint32_t, U32);
MAKE_METHOD(uint64_t, U64);
MAKE_METHOD(float, F32);
MAKE_METHOD(double, F64);
