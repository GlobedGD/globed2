#pragma once

namespace globed {

// adler-32 implementation suitable for hashing string literals at compile time.
// Hash does not include the null terminator.
template <size_t N> constexpr inline uint32_t adler32(const char (&data)[N])
{
    const uint32_t MOD = 65521;

    uint32_t a = 1, b = 0;

    // make sure to exclude the null terminator
    for (size_t i = 0; i < N - 1; i++) {
        a = (a + data[i]) % MOD;
        b = (b + a) % MOD;
    }

    return (b << 16) | a;
}

template <size_t N> struct ConstexprString {
    constexpr ConstexprString(const char (&str)[N])
    {
        std::copy_n(str, N, value);
        hash = adler32(str);
    }
    constexpr bool operator!=(const ConstexprString &other) const
    {
        return std::equal(value, value + N - 1, other.value);
    }
    constexpr operator std::string() const
    {
        return std::string(value, N - 1);
    }
    constexpr operator std::string_view() const
    {
        return std::string_view(value, N - 1);
    }
    char value[N];
    uint32_t hash;
};

template <> struct ConstexprString<0> {
    constexpr ConstexprString() {}
};

} // namespace globed