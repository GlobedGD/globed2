#include "gd.hpp"

#include <util/rng.hpp>

PlayerIconData PlayerIconData::makeRandom() {
    auto& rng = util::rng::Random::get();

    PlayerIconData data;
    data.cube = rng.generate(1, 484);
    data.ship = rng.generate(1, 168);
    data.ball = rng.generate(1, 118);
    data.ufo = rng.generate(1, 149);
    data.wave = rng.generate(1, 96);
    data.robot = rng.generate(1, 68);
    data.spider = rng.generate(1, 69);
    data.swing = rng.generate(1, 43);
    data.jetpack = rng.generate(1, 5);

    data.deathEffect = 0;
    data.color1 = rng.generate(1, 106);
    data.color2 = rng.generate(1, 106);
    data.glowColor = NO_GLOW;
    data.streak = NO_TRAIL;
    data.shipStreak = NO_TRAIL;

    return data;
}

PlayerPreviewAccountData PlayerPreviewAccountData::makeRandom() {
    PlayerPreviewAccountData data;

    auto& rng = util::rng::Random::get();

    data.accountId = rng.generate(10000, 10000000);
    data.userId = rng.generate(10000, 10000000);
    data.name = fmt::format("Player{}", data.accountId);
    data.icons = PlayerIconData::makeRandom();
    data.specialUserData = {};

    return data;
}

