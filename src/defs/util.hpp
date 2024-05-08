#pragma once
#include <stdexcept>

#include <config.hpp>
#include <embedded_resources.hpp>
#include "platform.hpp"

class singleton_use_after_dtor : public std::runtime_error {
public:
    singleton_use_after_dtor() : std::runtime_error("attempting to use a singleton after static destruction") {}
};

// singleton classes

// there was no reason to do this other than for me to learn crtp
template <typename Derived>
class SingletonBase {
public:
    // no copy
    SingletonBase(const SingletonBase&) = delete;
    SingletonBase& operator=(const SingletonBase&) = delete;
    // no move
    SingletonBase(SingletonBase&&) = delete;
    SingletonBase& operator=(SingletonBase&&) = delete;

    static Derived& get() {
        static Derived instance;

        if (destructed) {
            throw singleton_use_after_dtor();
        }

        return instance;
    }

protected:
    static inline bool destructed = false;

    SingletonBase() {}
    virtual ~SingletonBase() {
        destructed = true;
    }
};

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
    constexpr ConstexprFloat(unsigned char value) : value(static_cast<float>(value) / 255.f) {}
    constexpr ConstexprFloat(float value) : value(value) {}

    constexpr float asFloat() const {
        return value;
    }

    constexpr unsigned char asByte() const {
        return static_cast<unsigned char>(value * 255.f);
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

