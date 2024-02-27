/*
* GD-related data structures
*/

#pragma once
#include <data/bytebuffer.hpp>
#include "game.hpp"

using LevelId = int64_t;

class PlayerIconData {
public:
    static const PlayerIconData DEFAULT_ICONS;

    // wee woo
    PlayerIconData(
        int16_t cube, int16_t ship, int16_t ball, int16_t ufo, int16_t wave, int16_t robot, int16_t spider, int16_t swing, int16_t jetpack, int16_t deathEffect, int16_t color1, int16_t color2, int16_t glowColor
    ) : cube(cube), ship(ship), ball(ball), ufo(ufo), wave(wave), robot(robot), spider(spider), swing(swing), jetpack(jetpack), deathEffect(deathEffect), color1(color1), color2(color2), glowColor(glowColor) {}

    PlayerIconData() {}

    bool operator==(const PlayerIconData&) const = default;

    GLOBED_ENCODE {
        buf.writeI16(cube);
        buf.writeI16(ship);
        buf.writeI16(ball);
        buf.writeI16(ufo);
        buf.writeI16(wave);
        buf.writeI16(robot);
        buf.writeI16(spider);
        buf.writeI16(swing);
        buf.writeI16(jetpack);
        buf.writeI16(deathEffect);
        buf.writeI16(color1);
        buf.writeI16(color2);
        buf.writeI16(glowColor);
    }

    GLOBED_DECODE {
        cube = buf.readI16();
        ship = buf.readI16();
        ball = buf.readI16();
        ufo = buf.readI16();
        wave = buf.readI16();
        robot = buf.readI16();
        spider = buf.readI16();
        swing = buf.readI16();
        jetpack = buf.readI16();
        deathEffect = buf.readI16();
        color1 = buf.readI16();
        color2 = buf.readI16();
        glowColor = buf.readI16();
    }

    int16_t cube, ship, ball, ufo, wave, robot, spider, swing, jetpack, deathEffect, color1, color2, glowColor;
};

inline const PlayerIconData PlayerIconData::DEFAULT_ICONS = PlayerIconData(
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, -1
);

class SpecialUserData {
public:
    SpecialUserData(cocos2d::ccColor3B nameColor) : nameColor(nameColor) {}
    SpecialUserData() {}

    bool operator==(const SpecialUserData&) const = default;

    GLOBED_ENCODE {
        buf.writeColor3(nameColor);
    }

    GLOBED_DECODE {
        nameColor = buf.readColor3();
    }

    cocos2d::ccColor3B nameColor;
};

class PlayerPreviewAccountData {
public:
    PlayerPreviewAccountData(int32_t id, std::string name, int16_t cube, int16_t color1, int16_t color2, int16_t glowColor, int32_t levelId)
        : accountId(id), name(name), cube(cube), color1(color1), color2(color2), glowColor(glowColor) {}
    PlayerPreviewAccountData() {}

    GLOBED_ENCODE {
        buf.writeI32(accountId);
        buf.writeString(name);
        buf.writeI16(cube);
        buf.writeI16(color1);
        buf.writeI16(color2);
        buf.writeI16(glowColor);
    }

    GLOBED_DECODE {
        accountId = buf.readI32();
        name = buf.readString();
        cube = buf.readI16();
        color1 = buf.readI16();
        color2 = buf.readI16();
        glowColor = buf.readI16();
    }

    int32_t accountId;
    std::string name;
    int16_t cube, color1, color2, glowColor;
};

class PlayerRoomPreviewAccountData {
public:
    PlayerRoomPreviewAccountData(int32_t id, int32_t userId, std::string name, int16_t cube, int16_t color1, int16_t color2, int16_t glowColor, LevelId levelId)
        : accountId(id), userId(userId), name(name), cube(cube), color1(color1), color2(color2), glowColor(glowColor), levelId(levelId) {}
    PlayerRoomPreviewAccountData() {}

    GLOBED_ENCODE {
        buf.writeI32(accountId);
        buf.writeI32(userId);
        buf.writeString(name);
        buf.writeI16(cube);
        buf.writeI16(color1);
        buf.writeI16(color2);
        buf.writeI16(glowColor);
        buf.writePrimitive(levelId);
        if (nameColor.has_value()) {
            buf.writeBool(true);
            buf.writeColor3(nameColor.value());
        } else {
            buf.writeBool(false);
        }
    }

    GLOBED_DECODE {
        accountId = buf.readI32();
        userId = buf.readI32();
        name = buf.readString();
        cube = buf.readI16();
        color1 = buf.readI16();
        color2 = buf.readI16();
        glowColor = buf.readI16();
        levelId = buf.readPrimitive<LevelId>();
        if (buf.readBool()) {
            nameColor = buf.readColor3();
        }
    }

    int32_t accountId, userId;
    std::string name;
    int16_t cube, color1, color2, glowColor;
    LevelId levelId;
    std::optional<cocos2d::ccColor3B> nameColor;
};

class PlayerAccountData {
public:
    static const PlayerAccountData DEFAULT_DATA;

    PlayerAccountData(int32_t id, int32_t userId, const std::string_view name, const PlayerIconData& icons)
        : accountId(id), userId(userId), name(name), icons(icons) {}

    PlayerAccountData() {}

    bool operator==(const PlayerAccountData&) const = default;

    PlayerRoomPreviewAccountData makeRoomPreview(LevelId levelId) {
        return PlayerRoomPreviewAccountData(
            accountId, userId, name, icons.cube, icons.color1, icons.color2, icons.glowColor, levelId
        );
    }

    GLOBED_ENCODE {
        buf.writeI32(accountId);
        buf.writeI32(userId);
        buf.writeString(name);
        buf.writeValue(icons);
        buf.writeOptionalValue(specialUserData);
    }

    GLOBED_DECODE {
        accountId = buf.readI32();
        userId = buf.readI32();
        name = buf.readString();
        icons = buf.readValue<PlayerIconData>();
        specialUserData = buf.readOptionalValue<SpecialUserData>();
    }

    int32_t accountId, userId;
    std::string name;
    PlayerIconData icons;
    std::optional<SpecialUserData> specialUserData;
};

inline const PlayerAccountData PlayerAccountData::DEFAULT_DATA = PlayerAccountData(
    0,
    0,
    "Player",
    PlayerIconData::DEFAULT_ICONS
);

class AssociatedPlayerData {
public:
    AssociatedPlayerData(int accountId, PlayerData data) : accountId(accountId), data(data) {}
    AssociatedPlayerData() {}

    GLOBED_ENCODE {
        buf.writeI32(accountId);
        buf.writeValue(data);
    }

    GLOBED_DECODE {
        accountId = buf.readI32();
        data = buf.readValue<PlayerData>();
    }

    int accountId;
    PlayerData data;
};
