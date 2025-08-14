#pragma once
#include <globed/util/ConstexprString.hpp>
#include <stdint.h>
#include <stddef.h>

namespace globed {

constexpr inline const char* constantByHash(uint32_t hash) {
#define CONSTANT(k,v) case adler32(k): return (v);
    switch (hash) {
        CONSTANT("discord", "https://discord.gg/d56q5Dkdm3")
    }
#undef CONSTANT

    return "<invalid constant key>";
}

template <size_t N>
static inline constexpr const char* constant(const char (&str)[N]) {
    return constantByHash(adler32(str));
}

template <ConstexprString str>
static inline constexpr const char* constant() {
    return constantByHash(str.hash);
}

}