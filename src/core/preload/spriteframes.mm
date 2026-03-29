#include "spriteframes.hpp"
#include <optional>
#include <Geode/Geode.hpp>

#define CommentType CommentTypeDummy
#import <Foundation/Foundation.h>
#undef CommentType

using namespace geode::prelude;

namespace globed {

SpriteFrameData::~SpriteFrameData() {
    if (this->dictRoot) {
        CFRelease(this->dictRoot);
    }
}

// Helper to convert NSString to string_view without copying
static inline std::string_view to_view(NSString* s) {
    if (!s) return {};
    return std::string_view([s UTF8String], [s lengthOfBytesUsingEncoding:NSUTF8StringEncoding]);
}

template <typename T>
std::optional<T> parseNode(id node) {
    if (!node) return std::nullopt;

    if constexpr (std::is_same_v<T, float>) {
        return [node floatValue];
    } else if constexpr (std::is_same_v<T, int>) {
        return [node intValue];
    } else if constexpr (std::is_same_v<T, bool>) {
        return (bool)[node boolValue];
    } else if constexpr (std::is_same_v<T, CCPoint> || std::is_same_v<T, CCSize> || std::is_same_v<T, CCRect>) {
        if ([node isKindOfClass:[NSString class]]) {
            if constexpr (std::is_same_v<T, CCRect>) return parseCCRect(to_view(node));
            else return parseCCPoint<T>(to_view(node));
        }
    }
    return std::nullopt;
}

# define assign_or_bail(var, val) if (auto _res = (val)) { var = std::move(_res).value(); } else { invalid = true; }

bool parseSpriteFrameV0(NSDictionary* dict, SpriteFrame& sframe) {
    __block bool invalid = false;
    sframe.textureRotated = false;

    [dict enumerateKeysAndObjectsUsingBlock:^(NSString* key, id valueNode, BOOL* stop) {
        uint32_t keyHash = hashStringRuntime(to_view(key));

        switch (keyHash) {
            case STRING_HASH("x"): {
                assign_or_bail(sframe.textureRect.origin.x, parseNode<float>(valueNode));
            } break;

            case STRING_HASH("y"): {
                assign_or_bail(sframe.textureRect.origin.y, parseNode<float>(valueNode));
            } break;

            case STRING_HASH("width"): {
                assign_or_bail(sframe.textureRect.size.width, parseNode<float>(valueNode));
            } break;

            case STRING_HASH("height"): {
                assign_or_bail(sframe.textureRect.size.height, parseNode<float>(valueNode));
            } break;

            case STRING_HASH("offsetX"): {
                assign_or_bail(sframe.offset.x, parseNode<float>(valueNode));
            } break;

            case STRING_HASH("offsetY"): {
                assign_or_bail(sframe.offset.y, parseNode<float>(valueNode));
            } break;

            case STRING_HASH("originalWidth"): {
                assign_or_bail(sframe.sourceSize.width, (parseNode<int>(valueNode)));
                sframe.sourceSize.width = std::abs(sframe.sourceSize.width);
            } break;

            case STRING_HASH("originalHeight"): {
                assign_or_bail(sframe.sourceSize.height, (parseNode<int>(valueNode)));
                sframe.sourceSize.height = std::abs(sframe.sourceSize.height);
            } break;

            default: break;
        }
    }];

    return !invalid;
}

bool parseSpriteFrameV1_2(NSDictionary* dict, SpriteFrame& sframe, int format) {
    __block bool invalid = false;
    sframe.textureRotated = false;

    [dict enumerateKeysAndObjectsUsingBlock:^(NSString* key, id valueNode, BOOL* stop) {
        uint32_t keyHash = hashStringRuntime(to_view(key));

        switch (keyHash) {
            case STRING_HASH("frame"): {
                assign_or_bail(sframe.textureRect, parseNode<cocos2d::CCRect>(valueNode));
            } break;

            case STRING_HASH("rotated"): {
                if (format == 2) {
                    assign_or_bail(sframe.textureRotated, parseNode<bool>(valueNode));
                }
            } break;

            case STRING_HASH("offset"): {
                assign_or_bail(sframe.offset, parseNode<cocos2d::CCPoint>(valueNode));
            } break;

            case STRING_HASH("sourceSize"): {
                assign_or_bail(sframe.sourceSize, parseNode<cocos2d::CCSize>(valueNode));
            } break;

            default: break;
        }
    }];

    return !invalid;
}

bool parseSpriteFrameV3(NSDictionary* dict, SpriteFrame& sframe) {
    __block bool invalid = false;
    sframe.textureRotated = false;

    [dict enumerateKeysAndObjectsUsingBlock:^(NSString* key, id valueNode, BOOL* stop) {
        uint32_t keyHash = hashStringRuntime(to_view(key));

        switch (keyHash) {
            case STRING_HASH("spriteOffset"): {
                assign_or_bail(sframe.offset, parseNode<cocos2d::CCPoint>(valueNode));
            } break;

            case STRING_HASH("spriteSize"): {
                assign_or_bail(sframe.textureRect.size, parseNode<cocos2d::CCSize>(valueNode));
            } break;

            case STRING_HASH("spriteSourceSize"): {
                assign_or_bail(sframe.sourceSize, parseNode<cocos2d::CCSize>(valueNode));
            } break;

            case STRING_HASH("textureRect"): {
                if (auto tr = parseNode<cocos2d::CCRect>(valueNode)) {
                    sframe.textureRect.origin = tr->origin;
                } else {
                    invalid = true;
                }
            } break;

            case STRING_HASH("textureRotated"): {
                assign_or_bail(sframe.textureRotated, parseNode<bool>(valueNode));
            } break;

            case STRING_HASH("aliases"): {
                // It seems like aliases are never used by GD, so I cannot test this code, but in theory it should work..
                if ([valueNode isKindOfClass:[NSArray class]]) {
                    for (NSString* alias in (NSArray*)valueNode) {
                        sframe.aliases.push_back([alias UTF8String]);
                    }
                }
            } break;

            default: break;
        }
    }];

    return !invalid;
}

bool parseSpriteFrame(NSDictionary* dict, SpriteFrame& sframe, int format) {
    switch (format) {
        case 0: return parseSpriteFrameV0(dict, sframe);
        case 1: [[fallthrough]];
        case 2: return parseSpriteFrameV1_2(dict, sframe, format);
        case 3: return parseSpriteFrameV3(dict, sframe);
        default: std::unreachable(); // this is already handled by parseSpriteFrames
    }
}


Result<std::unique_ptr<SpriteFrameData>> parseSpriteFrames(void* data, size_t size, bool ownBuffer) {
    @autoreleasepool {
        NSData* nsData = [NSData dataWithBytesNoCopy:data length:size freeWhenDone:ownBuffer];
        NSError* error = nil;

        NSDictionary* root = [NSPropertyListSerialization
            propertyListWithData:nsData
            options:NSPropertyListImmutable
            format:nil
            error:&error
        ];
        if (error || !root) {
            return Err("NSPropertyList failed: {}", [[error localizedDescription] UTF8String]);
        }

        auto sfdata = std::make_unique<SpriteFrameData>();
        NSDictionary* metadata = root[@"metadata"];
        NSDictionary* frames = root[@"frames"];

        if (!metadata || !frames) {
            return Err("Invalid plist structure: missing 'metadata' or 'frames'");
        }

        // parse metadata
        sfdata->metadata.format = [metadata[@"format"] intValue];
        if (NSString* texName = metadata[@"textureFileName"]) {
            sfdata->metadata.textureFileName = [texName UTF8String];
        }

        if (sfdata->metadata.format < 0 || sfdata->metadata.format > 3) {
            return Err("Unsupported format version: {}", sfdata->metadata.format);
        }

        for (NSString* frameName in frames) {
            NSDictionary* frameDict = frames[frameName];

            SpriteFrame sframe;
            sframe.name = [frameName UTF8String];

            if (!parseSpriteFrame(frameDict, sframe, sfdata->metadata.format)) {
                log::warn("Failed to parse frame '{}', skipping!", sframe.name);
                continue;
            }

            sfdata->frames.push_back(std::move(sframe));
        }

        sfdata->dictRoot = (void*)CFRetain(root);

        return Ok(std::move(sfdata));
    }
}

}
