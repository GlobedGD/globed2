#pragma once

#include <config.hpp>
#include <embedded_resources.hpp>

// gets a string from embedded resources json
// "key"_gstr;
inline const char* operator ""_gstr(const char* key, size_t size) {
    return globed::stringbyhash(util::crypto::adler32(reinterpret_cast<const uint8_t*>(key), size));
}

namespace globed {

template <size_t N>
struct ConstexprString {
    constexpr ConstexprString(const char (&str)[N]) {
        std::copy_n(str, N, value);
        hash = util::crypto::adler32(str);
    }
    constexpr bool operator!=(const ConstexprString& other) const {
        return std::equal(value, value + N, other.value);
    }
    constexpr operator std::string() const {
        return std::string(value, N);
    }
    constexpr operator std::string_view() const {
        return std::string_view(value, N);
    }
    char value[N];
    uint32_t hash;
};

struct ConstexprFloat {
    constexpr ConstexprFloat(float value) : value(value) {}

    constexpr float asFloat() const {
        return value;
    }

    constexpr operator float() const {
        return this->asFloat();
    }

    float& ref() {
        return value;
    }

    const float& ref() const {
        return value;
    }

    float value;
};

// const char* x = globed::string("key");
template <size_t N>
static inline constexpr const char* string(const char (&str)[N]) {
    auto ret = globed::stringbyhash(util::crypto::adler32(str));
    return ret ? ret : "<invalid string>";
}

// const char* x = globed::string<"key">();
template <ConstexprString str>
static inline constexpr const char* string() {
    auto ret = globed::stringbyhash(str.hash);
    return ret ? ret : "<invalid string>";
}

}

