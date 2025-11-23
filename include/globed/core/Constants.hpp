#pragma once
#include <globed/util/ConstexprString.hpp>
#include <stdint.h>
#include <stddef.h>

namespace globed {

const char* constantByHash(uint32_t hash);

template <size_t N>
static inline constexpr const char* constant(const char (&str)[N]) {
    return constantByHash(adler32(str));
}

template <ConstexprString str>
static inline constexpr const char* constant() {
    return constantByHash(str.hash);
}

}