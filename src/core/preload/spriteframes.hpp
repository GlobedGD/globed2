#pragma once

// -- This implementation is copied from Blaze --

// Custom parsing for plist files containing cocos2d sprite frames.
// Does very minimal heap allocations, uses fast XML and float parsers.
// Proves to be ~8-9 times faster than the cocos2d implementation (on Windows)

#include <pugixml.hpp>

#include <Geode/Result.hpp>
#include <cocos2d.h>

namespace globed {

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

    pugi::xml_document doc;
    std::vector<SpriteFrame> frames;
};

// Parses data from a .plist file into a structure holding many sprite frames.
geode::Result<std::unique_ptr<SpriteFrameData>> parseSpriteFrames(void* data, size_t size, bool ownBuffer = false);

// Adds sprite frames to `CCSpriteFrameCache` from a parsed `SpriteFrameData`.
void addSpriteFrames(const SpriteFrameData& frames, cocos2d::CCTexture2D* texture);

// Adds sprite frames to `CCSpriteFrameCache` from a vanilla `CCDictionary`.
// Is not optimized, simply calls `CCSpriteFrameCache::addSpriteFramesWithDictionary`
void addSpriteFramesVanilla(cocos2d::CCDictionary* dict, cocos2d::CCTexture2D* texture);

}
