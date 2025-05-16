#pragma once

#include <defs/minimal_geode.hpp>
#include <data/bytebuffer.hpp>
#include <cocos2d.h>

class GameServerEntry {
public:
    std::string id, name, address, region;
};

GLOBED_SERIALIZABLE_STRUCT(GameServerEntry, (
    id, name, address, region
));

class GlobedLevel {
public:
    LevelId levelId;
    unsigned short playerCount;
};

GLOBED_SERIALIZABLE_STRUCT(GlobedLevel, (
    levelId, playerCount
));

class ErrorMessage {
public:
    constexpr ErrorMessage(uint32_t h) : hash(h) {}

    uint32_t hash;

    template <globed::ConstexprString str>
    constexpr static ErrorMessage create() {
        return ErrorMessage(str.hash);
    }

    template <size_t N>
    constexpr static ErrorMessage create(const char (&key)[N]) {
        return ErrorMessage(util::crypto::adler32(key));
    }

    constexpr static ErrorMessage fromHash(uint32_t hash) {
        return ErrorMessage(hash);
    }

    constexpr const char* message() const {
        return globed::stringbyhash(hash);
    }
};

GLOBED_SERIALIZABLE_STRUCT(ErrorMessage, (
    hash
));

class CustomErrorMessage {
public:
    Either<ErrorMessage, std::string> variant;

    CustomErrorMessage(ErrorMessage m) : variant(m) {}
    CustomErrorMessage(std::string_view s) : variant(std::string(s)) {}
    CustomErrorMessage(std::string&& s) : variant(std::move(s)) {}

    bool isCustom() const {
        return variant.isSecond();
    }

    std::string_view message() {
        if (variant.isFirst()) {
            return variant.firstRef()->get().message();
        } else {
            return variant.secondRef()->get();
        }
    }
};

GLOBED_SERIALIZABLE_STRUCT(CustomErrorMessage, (
    variant
));

class RichColor {
public:
    Either<cocos2d::ccColor3B, std::vector<cocos2d::ccColor3B>> inner;

    RichColor() : inner(cocos2d::ccColor3B{255, 255, 255}) {}
    RichColor(const cocos2d::ccColor3B& col) : inner(col) {}
    RichColor(const std::vector<cocos2d::ccColor3B>& col) : inner(col) {}
    RichColor(std::vector<cocos2d::ccColor3B>&& col) : inner(std::move(col)) {}

    static Result<RichColor> parse(std::string_view k);

    bool operator==(const RichColor& other) const = default;

    bool isMultiple() const;

    std::vector<cocos2d::ccColor3B>& getColors();
    const std::vector<cocos2d::ccColor3B>& getColors() const;

    cocos2d::ccColor3B getColor() const;
    cocos2d::ccColor3B getAnyColor() const;

    void animateLabel(cocos2d::CCLabelBMFont* label) const;
};

GLOBED_SERIALIZABLE_STRUCT(RichColor, (
    inner
));

struct ServerRelay {
    std::string id;
    std::string name;
    std::string address;
};
