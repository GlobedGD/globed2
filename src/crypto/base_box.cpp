#include "base_box.hpp"
#include <cstring> // std::memmove

#include <defs/assert.hpp>
#include <defs/minimal_geode.hpp>

using namespace util::data;

constexpr size_t BaseCryptoBox::prefixLength() {
    return nonceLength() + macLength();
}

size_t BaseCryptoBox::encryptInto(const std::string_view src, byte* dest) {
    return encryptInto(reinterpret_cast<const byte*>(src.data()), dest, src.size());
}

size_t BaseCryptoBox::encryptInto(const bytevector& src, byte* dest) {
    return encryptInto(src.data(), dest, src.size());
}

size_t BaseCryptoBox::encryptInPlace(byte* data, size_t size) {
    return encryptInto(data, data, size);
}

bytevector BaseCryptoBox::encrypt(const bytevector& src) {
    return encrypt(src.data(), src.size());
}

bytevector BaseCryptoBox::encrypt(const byte* src, size_t size) {
    bytevector output(size + prefixLength());
    encryptInto(src, output.data(), size);
    return output;
}

bytevector BaseCryptoBox::encrypt(const std::string_view src) {
    return encrypt(reinterpret_cast<const byte*>(src.data()), src.size());
}

size_t BaseCryptoBox::decryptInPlace(byte* data, size_t size) {
    // overwriting nonce causes decryption to break
    // so we offset the destination by NONCE_LEN and then move it back
    size_t plaintext_size = decryptInto(data, data + nonceLength(), size);

    std::memmove(data, data + nonceLength(), plaintext_size);

    return plaintext_size;
}

bytevector BaseCryptoBox::decrypt(const bytevector& src) {
    return decrypt(src.data(), src.size());
}

bytevector BaseCryptoBox::decrypt(const byte* src, size_t size) {
    size_t plaintextLength = size - prefixLength();

    bytevector plaintext(plaintextLength);
    decryptInto(src, plaintext.data(), size);

    return plaintext;
}

std::string BaseCryptoBox::decryptToString(const bytevector& src) {
    auto vec = decrypt(src);
    return std::string(vec.begin(), vec.end());
}

std::string BaseCryptoBox::decryptToString(const byte* src, size_t size) {
    auto vec = decrypt(src, size);
    return std::string(vec.begin(), vec.end());
}