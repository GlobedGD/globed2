#pragma once
#include <data/bytebuffer.hpp>
#include "gd.hpp"

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
