#pragma once
#include <data/bytebuffer.hpp>

enum class PlayerIconType : uint8_t {
    Unknown = 0,
    Cube = 1,
    Ship = 2,
    Ball = 3,
    Ufo = 4,
    Wave = 5,
    Robot = 6,
    Spider = 7,
    Swing = 8,
};

struct SpecificIconData {
    GLOBED_ENCODE {
        buf.writeEnum(iconType);
        buf.writePoint(position);
        buf.writeF32(rotation);
    }

    GLOBED_DECODE {
        iconType = buf.readEnum<PlayerIconType>();
        GLOBED_REQUIRE(iconType >= PlayerIconType::Unknown && iconType <= PlayerIconType::Swing, "invalid PlayerIconType value encountered when decoding SpecificIconData")
        if (iconType == PlayerIconType::Unknown) {
            iconType = PlayerIconType::Cube;
        }

        position = buf.readPoint();
        rotation = buf.readF32();
    }

    PlayerIconType iconType;
    cocos2d::CCPoint position;
    float rotation;
};

class PlayerData {
public:
    PlayerData(
        uint16_t percentage,
        int32_t attempts
    )
    : percentage(percentage),
      attempts(attempts)
    {}

    PlayerData() {}
    GLOBED_ENCODE {
        buf.writeU16(percentage);
        buf.writeI32(attempts);
        buf.writeValue(player1);
        buf.writeValue(player2);
    }

    GLOBED_DECODE {
        percentage = buf.readU16();
        attempts = buf.readI32();
        player1 = buf.readValue<SpecificIconData>();
        player2 = buf.readValue<SpecificIconData>();
    }

    uint16_t percentage;
    int32_t attempts;

    SpecificIconData player1;
    SpecificIconData player2;
};