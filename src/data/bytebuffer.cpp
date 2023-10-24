#include "bytebuffer.hpp"

using namespace util::data;

#define READ_VALUE(type) type val = this->read<type>(); \
    if constexpr (GLOBED_LITTLE_ENDIAN) val = util::data::byteswap<type>(val); \
    return val;

#define WRITE_VALUE(type, value) GLOBED_LITTLE_ENDIAN ? write<type>(util::data::byteswap<type>(value)) : write<type>(value);

#define MAKE_READ_FUNC(type, suffix) type ByteBuffer::read##suffix() { READ_VALUE(type) }
#define MAKE_WRITE_FUNC(type, suffix) void ByteBuffer::write##suffix(type val) { WRITE_VALUE(type, val) }

#define MAKE_BOTH_FUNCS(type, suffix) MAKE_READ_FUNC(type, suffix) \
    MAKE_WRITE_FUNC(type, suffix)

ByteBuffer::ByteBuffer() : _position(0) {}
ByteBuffer::ByteBuffer(const bytevector& data) : _data(data), _position(0) {}
ByteBuffer::ByteBuffer(const byte* data, size_t length) : _data(bytevector(data, data + length)), _position(0) {}

template <typename T>
T ByteBuffer::read() {
    boundsCheck(sizeof(T));

    T value;
    std::memcpy(&value, _data.data() + _position, sizeof(T));
    _position += sizeof(T);
    return value;
}

template <typename T>
void ByteBuffer::write(T value) {
    const byte* bytes = reinterpret_cast<const byte*>(&value);
    _data.insert(_data.end(), bytes, bytes + sizeof(T));
    _position += sizeof(T);
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
    auto length = readU32();

    boundsCheck(length);

    std::string str(reinterpret_cast<const char*>(_data.data() + _position), length);
    _position += length;
    return str;
}

void ByteBuffer::writeString(const std::string& str) {
    writeU32(str.size());
    _data.insert(_data.end(), str.begin(), str.end());
    _position += str.size();
}

std::vector<uint8_t> ByteBuffer::getData() const {
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