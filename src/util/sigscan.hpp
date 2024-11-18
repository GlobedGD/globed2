#pragma once

#include <defs/util.hpp>

namespace util::sigscan {

namespace _detail {
    constexpr bool isHexChar(char c) {
        return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
    }

    constexpr uint8_t hexToByte(char byte1, char byte2) {
        auto _pb = [](char c) {
            if (c >= '0' && c <= '9') {
                return c - '0';
            } else if (c >= 'A' && c <= 'F') {
                return 0xa + (c - 'A');
            } else if (c >= 'a' && c <= 'f') {
                return 0xa + (c - 'a');
            } else {
                return 0;
            }
        };

        return _pb(byte1) << 4 | _pb(byte2);
    }

    // Returns -1 on an invalid input.
    template <globed::ConstexprString str>
    constexpr size_t patSize() {
        size_t inarow = 0;
        size_t total = 0;

        auto assert_ = [&](bool cond) { if(!cond) total = -1; };

        for (char c : std::string_view(str)) {
            if (c == '\0') continue;

            if (c == ' ' && inarow == 2) {
                inarow = 0;
                total++;
            } else if (isHexChar(c) || c == '?') {
                inarow++;
            } else {
                return -1;
            }

            if (inarow > 2) {
                return -1;
            }
        }

        assert_(inarow == 2 || inarow == 0);
        if (inarow == 2) {
            total++;
        }

        return total;
    }
}

template <size_t N>
struct Pattern {
    constexpr static size_t Size = N;

    std::array<std::optional<uint8_t>, N> bytes;

    constexpr Pattern(std::array<std::optional<uint8_t>, N> arr) : bytes(arr) {}
};

template <globed::ConstexprString str, size_t N = _detail::patSize<str>()>
constexpr Pattern<N> pattern() {
    static_assert(N != -1, "invalid pattern, bytes must be separated by spaces");

    std::array<std::optional<uint8_t>, N> bytes;

    std::string_view sv{str};

    for (size_t i = 0; i < sv.size(); i += 3) {
        size_t byteidx = i / 3;
        if (sv.at(i) == '?' || sv.at(i + 1) == '?') {
            bytes[byteidx] = std::nullopt;
        } else {
            bytes[byteidx] = std::optional{_detail::hexToByte(sv.at(i), sv.at(i + 1))};
        }
    }

    return Pattern{bytes};
}

// Find the given pattern in the given address range, return -1 on failure
template <globed::ConstexprString str>
uintptr_t locateInRange(uintptr_t start, uintptr_t end) {
    Pattern pat = pattern<str>();

    for (uint8_t* startPtr = reinterpret_cast<uint8_t*>(start); (startPtr + decltype(pat)::Size) < reinterpret_cast<uint8_t*>(end); startPtr++) {
        bool broken = false;

        for (size_t i = 0; i < decltype(pat)::Size; i++) {
            if (pat.bytes[i] && startPtr[i] != *(pat.bytes[i])) {
                broken = true;
                break;
            }
        }

        if (!broken) {
            return reinterpret_cast<uintptr_t>(startPtr);
        }
    }

    return -1;
}

template <globed::ConstexprString str, size_t SearchSize = 0x1000>
uintptr_t find(uintptr_t start, size_t searchSize = SearchSize) {
    return locateInRange<str>(start, start + searchSize);
}

}
