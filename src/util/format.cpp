#include <globed/util/format.hpp>

using namespace geode;

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

Result<std::vector<uint8_t>> hexDecode(std::string_view data) {
    if (data.size() % 2 != 0) return Err("hex string length not even");

    std::vector<uint8_t> out;
    out.reserve(data.size() / 2 );

    auto dechb = [](char hexchar) -> std::optional<uint8_t> {
        if (hexchar >= '0' && hexchar <= '9') {
            return hexchar - '0';
        } else if (hexchar >= 'A' && hexchar <= 'F') {
            return hexchar - 'A' + 10;
        } else if (hexchar >= 'a' && hexchar <= 'f') {
            return hexchar - 'a' + 10;
        } else {
            return std::nullopt;
        }
    };

    for (size_t i = 0; i < data.size(); i++) {
        auto a = dechb(data[i]);
        auto b = dechb(data[i + 1]);

        if (!a || !b) {
            return Err("hex string has invalid character");
        }

        out.push_back((*a << 4) | *b);
    }

    return Ok(std::move(out));
}

}