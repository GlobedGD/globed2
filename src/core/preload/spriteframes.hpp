#pragma once

// -- This implementation is copied from Blaze --

// Custom parsing for plist files containing cocos2d sprite frames.
// Does very minimal heap allocations, uses fast XML and float parsers.
// Proves to be ~8-9 times faster than the cocos2d implementation (on Windows)

#include <pugixml.hpp>

#include <Geode/Result.hpp>
#include <Geode/utils/general.hpp>
#include <Geode/loader/Log.hpp>
#include <cocos2d.h>
#include <fmt/core.h>
#include <asp/iter.hpp>

// This is to ensure that Geode's pugixml header is not included
static_assert(std::is_trivially_destructible_v<pugi::xml_node>);

namespace globed {

constexpr uint32_t FNV_OFFSET_BASIS = 2166136261u;
constexpr uint32_t FNV_PRIME = 16777619u;

constexpr static inline uint32_t _fnv1a_hash(const char *str, uint32_t hash = FNV_OFFSET_BASIS) {
    return (*str == '\0') ? hash : _fnv1a_hash(str + 1, (hash ^ static_cast<uint8_t>(*str)) * FNV_PRIME);
}

#define STRING_HASH(x) (::globed::_fnv1a_hash(x))

static uint32_t hashStringRuntime(const char* str) {
    uint32_t hash = FNV_OFFSET_BASIS;

    while (*str) {
        hash = ((hash ^ static_cast<uint8_t>(*str++)) * FNV_PRIME);
    }

    return hash;
}

static uint32_t hashStringRuntime(std::string_view str) {
    uint32_t hash = FNV_OFFSET_BASIS;

    for (char c : str) {
        hash = ((hash ^ static_cast<uint8_t>(c)) * FNV_PRIME);
    }

    return hash;
}

struct SpriteFrame {
    const char* name = nullptr;
    cocos2d::CCPoint offset;
    // Apparently unused? (sourceSize attr in format v3)
    // cocos2d::CCSize size;
    cocos2d::CCSize sourceSize;
    cocos2d::CCRect textureRect;
    bool textureRotated = false;
    std::vector<const char*> aliases;
};

struct SpriteFrameData {
    struct Metadata {
        int format = -1;
        const char* textureFileName = "";
    } metadata;

    ~SpriteFrameData();

#ifndef __APPLE__
    pugi::xml_document doc;
#else
    void* dictRoot = nullptr;
#endif

    std::vector<SpriteFrame> frames;
};

// Parses data from a .plist file into a structure holding many sprite frames.
geode::Result<std::unique_ptr<SpriteFrameData>> parseSpriteFrames(void* data, size_t size, bool ownBuffer = false);

// Adds sprite frames to `CCSpriteFrameCache` from a parsed `SpriteFrameData`.
void addSpriteFrames(const SpriteFrameData& frames, cocos2d::CCTexture2D* texture);

// Adds sprite frames to `CCSpriteFrameCache` from a vanilla `CCDictionary`.
// Is not optimized, simply calls `CCSpriteFrameCache::addSpriteFramesWithDictionary`
void addSpriteFramesVanilla(cocos2d::CCDictionary* dict, cocos2d::CCTexture2D* texture);


std::optional<cocos2d::CCRect> parseCCRect(std::string_view str);

template <typename T = cocos2d::CCPoint>
std::optional<T> parseCCPoint(std::string_view str) {
    // A point is formatted in form {x,y}.
    // Cocos does a bunch of unnecessary checks here, we are just going to go by the following rules:
    // * First character has to be an opening brace
    // * First number is parsed after the first brace
    // * At the end of the first number, a comma must be present
    // * Second number is parsed after the comma
    // * At the end of the second number, a closing brace must be present
    float x = 0.f, y = 0.f;

    if (str.size() < 5) {
        return std::nullopt;
    }

    if (str[0] != '{') {
        return std::nullopt;
    }

    str.remove_prefix(1);
    auto parts = asp::iter::split(str, ',');
    auto firstStr = parts.next();
    auto secondStr = parts.next();

    if (!firstStr || !secondStr) {
        return std::nullopt;
    }

    auto result = geode::utils::numFromString<float>(*firstStr);
    if (!result) {
        return std::nullopt;
    }
    x = *result;

    // second num (has a brace at the end)

    if (!secondStr->ends_with('}')) {
        return std::nullopt;
    }
    secondStr->remove_suffix(1);
    result = geode::utils::numFromString<float>(*secondStr);
    if (!result) {
        return std::nullopt;
    }
    y = *result;

    return T{x, y};
}


}
