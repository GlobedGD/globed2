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
        int cube, int ship, int ball, int ufo, int wave, int robot, int spider, int swing, int jetpack, int deathEffect
    ) : cube(cube), ship(ship), ball(ball), ufo(ufo), wave(wave), robot(robot), spider(spider), swing(swing), jetpack(jetpack), deathEffect(deathEffect) {}

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
    }

    int cube, ship, ball, ufo, wave, robot, spider, swing, jetpack, deathEffect;
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
    }

    GLOBED_DECODE {
        id = buf.readI32();
        name = buf.readString();
        icons = buf.readValue<PlayerIconData>();
    }

    int32_t id;
    std::string name;
    PlayerIconData icons;
};