#pragma once

#include <cocos2d.h>

namespace globed {

constexpr uint8_t hexDigit(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    throw "invalid hex digit";
}

constexpr uint8_t hexByte(char hi, char lo) {
    return static_cast<uint8_t>((hexDigit(hi) << 4) | hexDigit(lo));
}

constexpr cocos2d::ccColor4B colorFromHex(const char* str) {
    if (str[0] == '#') str++;

    int len = 0;
    while (str[len] != '\0') ++len;
    if (len != 6 && len != 8) throw "Invalid hex color length";

    uint8_t r = hexByte(str[0], str[1]);
    uint8_t g = hexByte(str[2], str[3]);
    uint8_t b = hexByte(str[4], str[5]);
    uint8_t a = (len == 8) ? hexByte(str[6], str[7]) : 255;

    return {r, g, b, a};
}

}