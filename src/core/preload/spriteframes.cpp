#include "spriteframes.hpp"

#include <Geode/loader/Log.hpp>
#include <Geode/Result.hpp>
#include <Geode/Prelude.hpp>
#include <Geode/utils/general.hpp>
#include <fmt/core.h>
#include <asp/iter.hpp>

using namespace geode::prelude;

// big hack to call a private cocos function
namespace {
    template <typename TC>
    using priv_method_t = void(TC::*)(CCDictionary*, CCTexture2D*);

    template <typename TC, priv_method_t<TC> func>
    struct priv_caller {
        friend void _addSpriteFramesWithDictionary(CCDictionary* p1, CCTexture2D* p2) {
            auto* obj = CCSpriteFrameCache::sharedSpriteFrameCache();
            (obj->*func)(p1, p2);
        }
    };

    template struct priv_caller<CCSpriteFrameCache, &CCSpriteFrameCache::addSpriteFramesWithDictionary>;

    void _addSpriteFramesWithDictionary(CCDictionary* p1, CCTexture2D* p2);
}

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


template <typename T = CCPoint>
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

std::optional<CCRect> parseCCRect(std::string_view str) {
    // A rect is formatted as {{x,y},{w,h}}.
    // We are going to go by the following rules:
    // * First character has to be an opening brace, and last character has to be a closing brace
    // * We find the index of the second comma, and split the string into two substrings (omitting the very first and very last braces)
    // * Call parseCCPoint on both substrings and use their results

    CCPoint origin;
    CCSize size;

    if (str[0] != '{' || str[str.size() - 1] != '}') {
        return std::nullopt;
    }

    size_t secondCommaIdx = 0;
    bool isFirstComma = true;
    for (size_t i = 0; i < str.size(); i++) {
        char c = str[i];
        if (c == ',') {
            if (isFirstComma) {
                isFirstComma = false;
            } else {
                secondCommaIdx = i;
                break;
            }
        }
    }

    if (secondCommaIdx == 0) {
        return std::nullopt;
    }

    auto originStr = str.substr(1, secondCommaIdx - 1);
    auto sizeStr = str.substr(secondCommaIdx + 1, str.size() - secondCommaIdx - 2);

    if (auto x = parseCCPoint(originStr)) {
        origin = x.value();
    } else {
        return std::nullopt;
    }

    if (auto x = parseCCPoint<CCSize>(sizeStr)) {
        size = x.value();
    } else {
        return std::nullopt;
    }

    return CCRect{origin, size};
}

template <typename T>
std::optional<T> parseNode(pugi::xml_node node) {
    auto str = node.child_value();

    if constexpr (std::is_same_v<T, float>) {
        return geode::utils::numFromString<float>(str).ok();
    } else if constexpr (std::is_same_v<T, int>) {
        return geode::utils::numFromString<int>(str).ok();
    } else if constexpr (std::is_same_v<T, bool>) {
        return strncmp(node.name(), "true", 5) == 0;
    } else if constexpr (std::is_same_v<T, CCPoint> || std::is_same_v<T, CCSize>) {
        return parseCCPoint<T>(str);
    } else if constexpr(std::is_same_v<T, CCRect>) {
        return parseCCRect(str);
    } else {
        static_assert(std::is_void_v<T>, "unsupported type");
    }
}

#ifdef BLAZE_DEBUG
# define assign_or_bail(var, val) \
    if (auto _res = (val)) { \
        var = std::move(_res).value(); \
    } else { \
        log::warn("Failed to parse {} ({}:{})", #var, __FILE__, __LINE__); \
        return false; \
    } \

#else
# define assign_or_bail(var, val) if (auto _res = (val)) { var = std::move(_res).value(); } else { return false; }
#endif

bool parseSpriteFrameV0(pugi::xml_node node, SpriteFrame& sframe) {
    sframe.textureRotated = false;

    for (pugi::xml_node keyNode = node.child("key"); keyNode; keyNode = keyNode.next_sibling("key")) {
        auto keyName = keyNode.child_value();

        // the corresponding value node
        auto valueNode = keyNode.next_sibling();

        if (!valueNode) {
            continue;
        }

        auto keyHash = hashStringRuntime(keyName);
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
        }
    }

    return true;
}

bool parseSpriteFrameV1_2(pugi::xml_node node, SpriteFrame& sframe, int format) {
    for (pugi::xml_node keyNode = node.child("key"); keyNode; keyNode = keyNode.next_sibling("key")) {
        auto keyName = keyNode.child_value();

        // the corresponding value node
        auto valueNode = keyNode.next_sibling();

        if (!valueNode) {
            continue;
        }

        auto keyHash = hashStringRuntime(keyName);
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
        }
    }

    return true;
}

bool parseSpriteFrameV3(pugi::xml_node node, SpriteFrame& sframe) {
    for (pugi::xml_node keyNode = node.child("key"); keyNode; keyNode = keyNode.next_sibling("key")) {
        auto keyName = keyNode.child_value();

        // the corresponding value node
        auto valueNode = keyNode.next_sibling();

        if (!valueNode) {
            continue;
        }

        auto keyHash = hashStringRuntime(keyName);

        switch (keyHash) {
            case STRING_HASH("spriteOffset"): {
                assign_or_bail(sframe.offset, parseNode<cocos2d::CCPoint>(valueNode));
            } break;

            // Apparently unused?
            // case STRING_HASH("spriteSize"): {
            //     sframe.size = parseNode<cocos2d::CCSize>(valueNode);
            // } break;

            case STRING_HASH("spriteSourceSize"): {
                assign_or_bail(sframe.sourceSize, parseNode<cocos2d::CCSize>(valueNode));
            } break;

            case STRING_HASH("textureRect"): {
                assign_or_bail(sframe.textureRect, parseNode<cocos2d::CCRect>(valueNode));
            } break;

            case STRING_HASH("textureRotated"): {
                assign_or_bail(sframe.textureRotated, parseNode<bool>(valueNode));
            } break;

            case STRING_HASH("aliases"): {
                // It seems like aliases are never used by GD, so I cannot test this code, but in theory it should work..
                for (pugi::xml_node aliasNode = valueNode.child("string"); aliasNode; aliasNode = aliasNode.next_sibling("string")) {
                    sframe.aliases.push_back(aliasNode.child_value());
                }
            } break;
        }
    }

    return true;
}

bool parseSpriteFrame(pugi::xml_node node, SpriteFrame& sframe, int format) {
    switch (format) {
        case 0: return parseSpriteFrameV0(node, sframe);
        case 1: [[fallthrough]];
        case 2: return parseSpriteFrameV1_2(node, sframe, format);
        case 3: return parseSpriteFrameV3(node, sframe);
        default: std::unreachable(); // this is already handled by parseSpriteFrames
    }
}

Result<std::unique_ptr<SpriteFrameData>> parseSpriteFrames(void* data, size_t size, bool ownBuffer) {
    auto sfdata = std::make_unique<SpriteFrameData>();

    pugi::xml_parse_result result;

    if (ownBuffer) {
        result = sfdata->doc.load_buffer_inplace_own(data, size);
    } else {
        result = sfdata->doc.load_buffer_inplace(data, size);
    }

    if (!result) {
        return Err("Failed to parse XML: {}", result.description());
    }

    pugi::xml_node plist = sfdata->doc.child("plist");
    if (!plist) {
        return Err("Failed to find root <plist> node");
    }

    pugi::xml_node rootDict = plist.child("dict");
    if (!rootDict) {
        return Err("Failed to find root <dict> node");
    }

    pugi::xml_node frames, metadata;

    for (pugi::xml_node keyNode = rootDict.child("key"); keyNode; keyNode = keyNode.next_sibling("key")) {
        auto keyName = keyNode.child_value();

        // the corresponding value node
        auto valueNode = keyNode.next_sibling();

        if (!valueNode) {
            continue;
        }

        if (strcmp(keyName, "frames") == 0) {
            frames = valueNode;
        } else if (strcmp(keyName, "metadata") == 0) {
            metadata = valueNode;
        }
    }

    if (!frames) {
        return Err("Failed to find 'frames' node");
    }

    if (!metadata) {
        return Err("Failed to find 'metadata' node");
    }

    // Iterate over the metadata
    for (pugi::xml_node keyNode = metadata.child("key"); keyNode; keyNode = keyNode.next_sibling("key")) {
        auto keyName = keyNode.child_value();

        // the corresponding value node
        auto valueNode = keyNode.next_sibling();

        if (!valueNode) {
            continue;
        }

        auto keyHash = hashStringRuntime(keyName);

        switch (keyHash) {
            case STRING_HASH("format"): {
                sfdata->metadata.format = parseNode<int>(valueNode).value_or(-1);
            } break;

            case STRING_HASH("textureFileName"): {
                sfdata->metadata.textureFileName = valueNode.child_value();
            } break;
        }
    }

    if (sfdata->metadata.format < 0 || sfdata->metadata.format > 3) {
        return Err("Unsupported format version: {}", sfdata->metadata.format);
    }

    // Note: one could try and optimize this by counting the amount of children and reserving space in the `sfData->frames`.
    // In my tests this proved to be ever so slightly slower, although it could depend on the platform and device.
    // Therefore this optimization was not done here.

    // Iterate over the frames
    for (pugi::xml_node keyNode = frames.child("key"); keyNode; keyNode = keyNode.next_sibling("key")) {
        auto frameKey = keyNode.child_value();

        // the corresponding value node
        auto frameDict = keyNode.next_sibling();

        if (!frameDict) {
            continue;
        }

        SpriteFrame frame;
        frame.name = frameKey;

        if (!parseSpriteFrame(frameDict, frame, sfdata->metadata.format)) {
            log::warn("Failed to parse frame '{}', skipping!", frameKey);
            continue;
        }

        sfdata->frames.push_back(std::move(frame));
    }

    return Ok(std::move(sfdata));
}

void addSpriteFrames(const SpriteFrameData& frames, cocos2d::CCTexture2D* texture) {
    auto sfcache = CCSpriteFrameCache::get();

    for (const auto& frame : frames.frames) {
        // create sprite frame
        auto spriteFrame = new CCSpriteFrame();
        bool result = spriteFrame->initWithTexture(
            texture,
            frame.textureRect,
            frame.textureRotated,
            frame.offset,
            frame.sourceSize
        );

        if (!result) {
            spriteFrame->release();
            log::warn("Failed to initialize sprite frame for {}", frame.name);
            continue;
        }

        // add sprite frame
        sfcache->m_pSpriteFrames->setObject(spriteFrame, frame.name);

        // if there are any aliases, add them as well
        if (!frame.aliases.empty()) {
            // create one CCString and reuse it
            auto fnamestr = CCString::create(frame.name);

            for (const auto& alias : frame.aliases) {
                sfcache->m_pSpriteFramesAliases->setObject(
                    fnamestr,
                    alias
                );
            }
        }

        spriteFrame->release();
    }

    sfcache->m_pLoadedFileNames->insert(frames.metadata.textureFileName);
}

void addSpriteFramesVanilla(cocos2d::CCDictionary* dict, cocos2d::CCTexture2D* texture) {
    _addSpriteFramesWithDictionary(dict, texture);
}

} // namespace blaze
