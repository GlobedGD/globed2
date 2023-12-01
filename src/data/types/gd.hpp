/*
* GD-related data structures
*/

#pragma once
#include <data/bytebuffer.hpp>
#include <util/data.hpp>

class PlayerIconData {
public:
    // wee woo
    PlayerIconData(
        int16_t cube, int16_t ship, int16_t ball, int16_t ufo, int16_t wave, int16_t robot, int16_t spider, int16_t swing, int16_t jetpack, int16_t deathEffect, int16_t color1, int16_t color2
    ) : cube(cube), ship(ship), ball(ball), ufo(ufo), wave(wave), robot(robot), spider(spider), swing(swing), jetpack(jetpack), deathEffect(deathEffect), color1(color1), color2(color2) {}

    PlayerIconData() {}

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
    }

    int16_t cube, ship, ball, ufo, wave, robot, spider, swing, jetpack, deathEffect, color1, color2;
};

class SpecialUserData {
public:
    SpecialUserData(cocos2d::ccColor3B nameColor) : nameColor(nameColor) {}
    SpecialUserData() {}

    GLOBED_ENCODE {
        buf.writeColor3(nameColor);
    }

    GLOBED_DECODE {
        nameColor = buf.readColor3();
    }

    cocos2d::ccColor3B nameColor;
};

class PlayerAccountData {
public:
    PlayerAccountData(int32_t id, const std::string& name, const PlayerIconData& icons)
        : id(id), name(name), icons(icons) {}

    PlayerAccountData() {}

    GLOBED_ENCODE {
        buf.writeI32(id);
        buf.writeString(name);
        buf.writeValue(icons);
        buf.writeOptionalValue(specialUserData);
    }

    GLOBED_DECODE {
        id = buf.readI32();
        name = buf.readString();
        icons = buf.readValue<PlayerIconData>();
        specialUserData = buf.readOptionalValue<SpecialUserData>();
    }

    int32_t id;
    std::string name;
    PlayerIconData icons;
    std::optional<SpecialUserData> specialUserData;
};

class PlayerPreviewAccountData {
public:
    PlayerPreviewAccountData(int32_t id, std::string name, int16_t cube, int16_t color1, int16_t color2, int32_t levelId)
        : id(id), name(name), cube(cube), color1(color1), color2(color2), levelId(levelId) {}
    PlayerPreviewAccountData() {}

    GLOBED_ENCODE {
        buf.writeI32(id);
        buf.writeString(name);
        buf.writeI16(cube);
        buf.writeI16(color1);
        buf.writeI16(color2);
        buf.writeI32(levelId);
    }

    GLOBED_DECODE {
        id = buf.readI32();
        name = buf.readString();
        cube = buf.readI16();
        color1 = buf.readI16();
        color2 = buf.readI16();
        levelId = buf.readI32();
    }

    int32_t id;
    std::string name;
    int16_t cube, color1, color2;
    int32_t levelId;
};

class PlayerData {
public:
    PlayerData() {}
    GLOBED_ENCODE {}
    GLOBED_DECODE {}
};

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

class PlayerMetadata {
public:
    PlayerMetadata(uint16_t percentage, int32_t attempts) : percentage(percentage), attempts(attempts) {}
    PlayerMetadata() {}

    GLOBED_ENCODE {
        buf.writeU16(percentage);
        buf.writeI32(attempts);
    }

    GLOBED_DECODE {
        percentage = buf.readU16();
        attempts = buf.readI32();
    }

    uint16_t percentage;
    int32_t attempts;
};

class AssociatedPlayerMetadata {
public:
    AssociatedPlayerMetadata(int accountId, PlayerMetadata data) : accountId(accountId), data(data) {}
    AssociatedPlayerMetadata() {}

    GLOBED_ENCODE {
        buf.writeI32(accountId);
        buf.writeValue(data);
    }

    GLOBED_DECODE {
        accountId = buf.readI32();
        data = buf.readValue<PlayerMetadata>();
    }

    int accountId;
    PlayerMetadata data;
};