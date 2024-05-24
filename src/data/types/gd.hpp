/*
* GD-related data structures
*/

#pragma once

#include <Geode/utils/cocos.hpp>

#include <data/bytebuffer.hpp>
#include "user.hpp"
#include "game.hpp"

class PlayerIconData {
public:
    static const PlayerIconData DEFAULT_ICONS;

    // wee woo
    PlayerIconData(
        int16_t cube, int16_t ship, int16_t ball, int16_t ufo, int16_t wave, int16_t robot, int16_t spider, int16_t swing, int16_t jetpack, int16_t deathEffect, int16_t color1, int16_t color2, int16_t glowColor
    ) : cube(cube), ship(ship), ball(ball), ufo(ufo), wave(wave), robot(robot), spider(spider), swing(swing), jetpack(jetpack), deathEffect(deathEffect), color1(color1), color2(color2), glowColor(glowColor) {}

    PlayerIconData() {}

    bool operator==(const PlayerIconData&) const = default;

    int16_t cube, ship, ball, ufo, wave, robot, spider, swing, jetpack, deathEffect, color1, color2, glowColor;
};

GLOBED_SERIALIZABLE_STRUCT(PlayerIconData, (
    cube, ship, ball, ufo, wave, robot, spider, swing, jetpack, deathEffect, color1, color2, glowColor
));

inline const PlayerIconData PlayerIconData::DEFAULT_ICONS = PlayerIconData(
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 3, -1
);

struct SpecialUserData {
    SpecialUserData() {}

    bool operator==(const SpecialUserData&) const = default;

    std::optional<std::vector<uint8_t>> roles;
};

GLOBED_SERIALIZABLE_STRUCT(SpecialUserData, (
    roles
));

class PlayerRoomPreviewAccountData {
public:
    PlayerRoomPreviewAccountData(int32_t id, int32_t userId, std::string name, int16_t cube, int16_t color1, int16_t color2, int16_t glowColor, LevelId levelId, const SpecialUserData& specialUserData)
        : accountId(id), userId(userId), name(name), cube(cube), color1(color1), color2(color2), glowColor(glowColor), levelId(levelId), specialUserData(specialUserData) {}
    PlayerRoomPreviewAccountData() {}

    int32_t accountId, userId;
    std::string name;
    int16_t cube, color1, color2, glowColor;
    LevelId levelId;
    SpecialUserData specialUserData;
};

GLOBED_SERIALIZABLE_STRUCT(PlayerRoomPreviewAccountData, (
    accountId, userId, name, cube, color1, color2, glowColor, levelId, specialUserData
));

class PlayerPreviewAccountData {
public:
    PlayerPreviewAccountData(int32_t id, int32_t userId, std::string name, int16_t cube, int16_t color1, int16_t color2, int16_t glowColor, int32_t levelId)
        : accountId(id), userId(userId), name(name), cube(cube), color1(color1), color2(color2), glowColor(glowColor) {}
    PlayerPreviewAccountData() {}

    int32_t accountId, userId;
    std::string name;
    int16_t cube, color1, color2, glowColor;
    SpecialUserData specialUserData;

    PlayerRoomPreviewAccountData makeRoomPreview() const {
        return PlayerRoomPreviewAccountData(
            accountId, userId,
            name,
            cube, color1, color2, glowColor,
            0, specialUserData
        );
    }
};

GLOBED_SERIALIZABLE_STRUCT(PlayerPreviewAccountData, (
    accountId, userId, name, cube, color1, color2, glowColor, specialUserData
));

class PlayerAccountData {
public:
    static const PlayerAccountData DEFAULT_DATA;

    PlayerAccountData(int32_t id, int32_t userId, const std::string_view name, const PlayerIconData& icons)
        : accountId(id), userId(userId), name(name), icons(icons) {}

    PlayerAccountData() {}

    bool operator==(const PlayerAccountData&) const = default;

    PlayerRoomPreviewAccountData makeRoomPreview(LevelId levelId) const {
        return PlayerRoomPreviewAccountData(
            accountId, userId, name, icons.cube, icons.color1, icons.color2, icons.glowColor, levelId, specialUserData
        );
    }

    int32_t accountId, userId;
    std::string name;
    PlayerIconData icons;
    SpecialUserData specialUserData;
};

GLOBED_SERIALIZABLE_STRUCT(PlayerAccountData, (
    accountId, userId, name, icons, specialUserData
));

inline const PlayerAccountData PlayerAccountData::DEFAULT_DATA = PlayerAccountData(
    0,
    0,
    "Player",
    PlayerIconData::DEFAULT_ICONS
);

class AssociatedPlayerData {
public:
    AssociatedPlayerData(int accountId, const PlayerData& data) : accountId(accountId), data(data) {}
    AssociatedPlayerData() {}

    int accountId;
    PlayerData data;
};

GLOBED_SERIALIZABLE_STRUCT(AssociatedPlayerData, (
    accountId, data
));

class AssociatedPlayerMetadata {
public:
    AssociatedPlayerMetadata(int accountId, const PlayerMetadata& data) : accountId(accountId), data(data) {}
    AssociatedPlayerMetadata() {}

    int accountId;
    PlayerMetadata data;
};

GLOBED_SERIALIZABLE_STRUCT(AssociatedPlayerMetadata, (
    accountId, data
));
