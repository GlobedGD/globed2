#include <globed/util/format.hpp>

namespace globed {

std::string hexEncode(const void* data, size_t size) {
    auto bytes = static_cast<const uint8_t*>(data);
    std::string result;
    result.reserve(size * 2);

    for (size_t i = 0; i < size; ++i) {
        result.push_back("0123456789abcdef"[bytes[i] >> 4]);
        result.push_back("0123456789abcdef"[bytes[i] & 0x0F]);
    }

    return result;
}

}