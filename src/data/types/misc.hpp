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
    CustomErrorMessage(const std::string_view s) : variant(std::string(s)) {}
    CustomErrorMessage(std::string&& s) : variant(std::move(s)) {}

    bool isCustom() const {
        return variant.isSecond();
    }

    const std::string_view message() {
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

    static Result<RichColor> parse(const std::string_view k);

    bool operator==(const RichColor& other) const = default;

    bool isMultiple() const {
        return inner.isSecond();
    }

    std::vector<cocos2d::ccColor3B>& getColors() {
        GLOBED_REQUIRE(inner.isSecond(), "calling RichColor::getColors when there is only 1 color");

        return inner.secondRef()->get();
    }

    const std::vector<cocos2d::ccColor3B>& getColors() const {
        GLOBED_REQUIRE(inner.isSecond(), "calling RichColor::getColors when there is only 1 color");

        return inner.secondRef()->get();
    }

    cocos2d::ccColor3B getColor() const {
        GLOBED_REQUIRE(inner.isFirst(), "calling RichColor::getColor when there are multiple colors");

        return inner.firstRef()->get();
    }

    cocos2d::ccColor3B getAnyColor() const {
        if (inner.isFirst()) {
            return inner.firstRef()->get();
        } else {
            auto& colors = inner.secondRef()->get();

            if (colors.empty()) {
                return cocos2d::ccc3(255, 255, 255);
            } else {
                return colors.at(0);
            }
        }
    }
};

GLOBED_SERIALIZABLE_STRUCT(RichColor, (
    inner
));
