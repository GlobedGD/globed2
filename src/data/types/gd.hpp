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
        int cube, int ship, int ball, int ufo, int wave, int robot, int spider, int swing, int jetpack, int deathEffect, int color1, int color2
    ) : cube(cube), ship(ship), ball(ball), ufo(ufo), wave(wave), robot(robot), spider(spider), swing(swing), jetpack(jetpack), deathEffect(deathEffect), color1(color1), color2(color2) {}

    PlayerIconData() {}

    GLOBED_ENCODE {
        buf.writeI32(cube);
        buf.writeI32(ship);
        buf.writeI32(ball);
        buf.writeI32(ufo);
        buf.writeI32(wave);
        buf.writeI32(robot);
        buf.writeI32(spider);
        buf.writeI32(swing);
        buf.writeI32(jetpack);
        buf.writeI32(deathEffect);
        buf.writeI32(color1);
        buf.writeI32(color2);
    }

    GLOBED_DECODE {
        cube = buf.readI32();
        ship = buf.readI32();
        ball = buf.readI32();
        ufo = buf.readI32();
        wave = buf.readI32();
        robot = buf.readI32();
        spider = buf.readI32();
        swing = buf.readI32();
        jetpack = buf.readI32();
        deathEffect = buf.readI32();
        color1 = buf.readI32();
        color2 = buf.readI32();
    }

    int cube, ship, ball, ufo, wave, robot, spider, swing, jetpack, deathEffect, color1, color2;
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