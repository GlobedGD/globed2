/*
* GD-related data structures
*/

#pragma once

#include <Geode/utils/cocos.hpp>

#include <data/bytebuffer.hpp>
#include "user.hpp"
#include "game.hpp"

static constexpr uint8_t NO_GLOW = -1;
static constexpr uint8_t NO_TRAIL = 1;

class PlayerIconData {
public:
    static const PlayerIconData DEFAULT_ICONS;

    // wee woo
    PlayerIconData(
        int16_t cube, int16_t ship, int16_t ball, int16_t ufo, int16_t wave, int16_t robot, int16_t spider, int16_t swing, int16_t jetpack, uint8_t deathEffect, uint8_t color1, uint8_t color2, uint8_t glowColor, uint8_t streak, uint8_t shipStreak
    ) : cube(cube), ship(ship), ball(ball), ufo(ufo), wave(wave), robot(robot), spider(spider), swing(swing), jetpack(jetpack), deathEffect(deathEffect), color1(color1), color2(color2), glowColor(glowColor), streak(streak), shipStreak(shipStreak) {}

    PlayerIconData() : PlayerIconData(PlayerIconData::DEFAULT_ICONS) {}
    PlayerIconData(const PlayerIconData&) = default;

    bool operator==(const PlayerIconData&) const = default;

    int16_t cube, ship, ball, ufo, wave, robot, spider, swing, jetpack;
    uint8_t deathEffect, color1, color2, glowColor, streak, shipStreak;
};

GLOBED_SERIALIZABLE_STRUCT(PlayerIconData, (
    cube, ship, ball, ufo, wave, robot, spider, swing, jetpack, deathEffect, color1, color2, glowColor, streak, shipStreak
));

inline const PlayerIconData PlayerIconData::DEFAULT_ICONS = PlayerIconData(
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 3, NO_GLOW, NO_TRAIL, NO_TRAIL
);

class PlayerIconDataSimple {
public:
    PlayerIconDataSimple(
        int16_t cube, uint8_t color1, uint8_t color2, uint8_t glowColor
    ) : cube(cube), color1(color1), color2(color2), glowColor(glowColor) {}
    PlayerIconDataSimple() : cube(1), color1(1), color2(3), glowColor(NO_GLOW) {}

    PlayerIconDataSimple(GJUserScore* score) :
        PlayerIconDataSimple(
            score->m_playerCube,
            score->m_color1,
            score->m_color2,
            score->m_glowEnabled ? (uint8_t)score->m_color3 : NO_GLOW
        ) {}

    PlayerIconDataSimple(const PlayerIconData& data) : PlayerIconDataSimple(data.cube, data.color1, data.color2, data.glowColor) {}

    int16_t cube;
    uint8_t color1, color2, glowColor;
};

GLOBED_SERIALIZABLE_STRUCT(PlayerIconDataSimple, (
    cube, color1, color2, glowColor
));

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
    PlayerRoomPreviewAccountData(int32_t id, int32_t userId, std::string name, PlayerIconDataSimple icons, LevelId levelId, const SpecialUserData& specialUserData)
        : accountId(id), userId(userId), name(name), icons(icons), levelId(levelId), specialUserData(specialUserData) {}
    PlayerRoomPreviewAccountData() {}

    int32_t accountId, userId;
    std::string name;
    PlayerIconDataSimple icons;
    LevelId levelId;
    SpecialUserData specialUserData;
};

GLOBED_SERIALIZABLE_STRUCT(PlayerRoomPreviewAccountData, (
    accountId, userId, name, icons, levelId, specialUserData
));

class PlayerPreviewAccountData {
public:
    PlayerPreviewAccountData(int32_t id, int32_t userId, std::string name, PlayerIconDataSimple icons, int32_t levelId)
        : accountId(id), userId(userId), name(name), icons(icons) {}
    PlayerPreviewAccountData() {}

    int32_t accountId, userId;
    std::string name;
    PlayerIconDataSimple icons;
    SpecialUserData specialUserData;

    PlayerRoomPreviewAccountData makeRoomPreview() const {
        return PlayerRoomPreviewAccountData(
            accountId, userId,
            name,
            icons,
            0, specialUserData
        );
    }
};

GLOBED_SERIALIZABLE_STRUCT(PlayerPreviewAccountData, (
    accountId, userId, name, icons, specialUserData
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
            accountId, userId, name, PlayerIconDataSimple(
                icons.cube,
                icons.color1,
                icons.color2,
                icons.glowColor
            ), levelId, specialUserData
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
