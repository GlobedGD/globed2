#include "bytebuffer.hpp"

#include <defs/assert.hpp>
#include <defs/minimal_geode.hpp>
#include <cstring> // std::memcpy

using namespace util::data;

#define READ_VALUE(type) return util::data::maybeByteswap<type>(this->read<type>());

#define WRITE_VALUE(type, value) this->write<type>(util::data::maybeByteswap<type>(value));

#define MAKE_READ_FUNC(type, suffix) \
    type ByteBuffer::read##suffix() { READ_VALUE(type) } \
    template type ByteBuffer::read<type>();

#define MAKE_WRITE_FUNC(type, suffix) \
    void ByteBuffer::write##suffix(type val) { WRITE_VALUE(type, val) } \
    template void ByteBuffer::write<type>(type);

#define MAKE_BOTH_FUNCS(type, suffix) MAKE_READ_FUNC(type, suffix) \
    MAKE_WRITE_FUNC(type, suffix)

ByteBuffer::ByteBuffer() : _position(0) {}
ByteBuffer::ByteBuffer(const bytevector& data) : _data(data), _position(0) {}
ByteBuffer::ByteBuffer(const byte* data, size_t length) : _data(bytevector(data, data + length)), _position(0) {}
ByteBuffer::ByteBuffer(util::data::bytevector&& data) : _data(std::move(data)), _position(0) {}

template <typename T>
T ByteBuffer::read() {
    this->boundsCheck(sizeof(T));

    T value;
    std::memcpy(&value, _data.data() + _position, sizeof(T));
    _position += sizeof(T);
    return value;
}

template <typename T>
void ByteBuffer::write(T value) {
    const byte* bytes = reinterpret_cast<const byte*>(&value);

    // if we can't fit (i.e. writing at the end, just use insert)
    if (_position + sizeof(T) > _data.size()) {
        _data.insert(_data.begin() + _position, bytes, bytes + sizeof(T));
        // remove leftover elements
        _data.reserve(_position + sizeof(T));
    } else {
        // otherwise, overwrite existing elements
        for (size_t i = 0; i < sizeof(T); i++) {
            _data.data()[_position + i] = bytes[i];
        }
    }

    _position += sizeof(T);
}

bool ByteBuffer::readBool() {
    return this->readU8() != 0;
}

void ByteBuffer::writeBool(bool value) {
    this->writeU8(value ? 1 : 0);
}

MAKE_BOTH_FUNCS(uint8_t, U8)
MAKE_BOTH_FUNCS(int8_t, I8)
MAKE_BOTH_FUNCS(uint16_t, U16)
MAKE_BOTH_FUNCS(int16_t, I16)
MAKE_BOTH_FUNCS(uint32_t, U32)
MAKE_BOTH_FUNCS(int32_t, I32)
MAKE_BOTH_FUNCS(uint64_t, U64)
MAKE_BOTH_FUNCS(int64_t, I64)
MAKE_BOTH_FUNCS(float, F32)
MAKE_BOTH_FUNCS(double, F64)

std::string ByteBuffer::readString() {
    auto length = this->readU32();

    this->boundsCheck(length);

    std::string str(reinterpret_cast<const char*>(_data.data() + _position), length);
    _position += length;

    return str;
}

bytevector ByteBuffer::readByteArray() {
    auto length = this->readU32();
    return this->readBytes(length);
}

bytevector ByteBuffer::readBytes(size_t size) {
    this->boundsCheck(size);

    bytevector vec(_data.begin() + _position, _data.begin() + _position + size);
    _position += size;

    return vec;
}

void ByteBuffer::readBytesInto(byte* out, size_t size) {
    this->boundsCheck(size);
    std::memcpy(out, _data.data() + _position, size);
    _position += size;
}

void ByteBuffer::writeString(const std::string_view str) {
    this->writeU32(str.size());
    _data.insert(_data.end(), str.begin(), str.end());
    _position += str.size();
}

void ByteBuffer::writeByteArray(const bytevector& vec) {
    this->writeU32(vec.size());
    this->writeBytes(vec);
}

void ByteBuffer::writeByteArray(const byte* data, size_t length) {
    this->writeU32(length);
    this->writeBytes(data, length);
}

void ByteBuffer::writeBytes(const util::data::byte* data, size_t size) {
    _data.insert(_data.end(), data, data + size);
    _position += size;
}

void ByteBuffer::writeBytes(const bytevector& vec) {
    this->writeBytes(vec.data(), vec.size());
}

/* cocos/gd */

cocos2d::ccColor3B ByteBuffer::readColor3() {
    auto r = this->readU8();
    auto g = this->readU8();
    auto b = this->readU8();
    return cocos2d::ccc3(r, g, b);
}

cocos2d::ccColor4B ByteBuffer::readColor4() {
    auto r = this->readU8();
    auto g = this->readU8();
    auto b = this->readU8();
    auto a = this->readU8();
    return cocos2d::ccc4(r, g, b, a);
}

cocos2d::CCPoint ByteBuffer::readPoint() {
    float x = this->readF32();
    float y = this->readF32();
    return ccp(x, y);
}

void ByteBuffer::writeColor3(cocos2d::ccColor3B color) {
    this->writeU8(color.r);
    this->writeU8(color.g);
    this->writeU8(color.b);
}

void ByteBuffer::writeColor4(cocos2d::ccColor4B color) {
    this->writeU8(color.r);
    this->writeU8(color.g);
    this->writeU8(color.b);
    this->writeU8(color.a);
}

void ByteBuffer::writePoint(cocos2d::CCPoint point) {
    this->writeF32(point.x);
    this->writeF32(point.y);
}

bytevector ByteBuffer::getData() const {
    return _data;
}

bytevector& ByteBuffer::getDataRef() {
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
}

void ByteBuffer::setPosition(size_t pos) {
    _position = pos;
}

void ByteBuffer::resize(size_t bytes) {
    _data.resize(bytes);
}

void ByteBuffer::grow(size_t bytes) {
    this->resize(_data.size() + bytes);
}

void ByteBuffer::shrink(size_t bytes) {
    this->resize(_data.size() - bytes);
}

void ByteBuffer::boundsCheck(size_t readBytes) {
    GLOBED_REQUIRE(_position + readBytes <= _data.size(), "ByteBuffer out of bounds read")
}