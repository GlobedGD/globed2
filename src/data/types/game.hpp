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
    Jetpack = 9,
};

struct SpecificIconData {
    GLOBED_ENCODE {
        buf.writeEnum(iconType);
        buf.writePoint(position);
        buf.writeF32(rotation);

        buf.writeBits(BitBuffer<8>(isVisible, isLookingLeft, isUpsideDown, isDashing));
    }

    GLOBED_DECODE {
        iconType = buf.readEnum<PlayerIconType>();
        GLOBED_REQUIRE(iconType >= PlayerIconType::Unknown && iconType <= PlayerIconType::Swing, "invalid PlayerIconType value encountered when decoding SpecificIconData")
        if (iconType == PlayerIconType::Unknown) {
            iconType = PlayerIconType::Cube;
        }

        position = buf.readPoint();
        rotation = buf.readF32();

        buf.readBits<8>().readBitsInto(isVisible, isLookingLeft, isUpsideDown, isDashing);
    }

    PlayerIconType iconType;
    cocos2d::CCPoint position;
    float rotation;

    bool isVisible;
    bool isLookingLeft;
    bool isUpsideDown;
    bool isDashing;
};

struct PlayerData {
    GLOBED_ENCODE {
        buf.writeF32(timestamp);
        buf.writeU16(percentage);
        buf.writeI32(attempts);

        buf.writeValue(player1);
        buf.writeValue(player2);

        buf.writeF32(lastDeathTimestamp);

        buf.writeBits(BitBuffer<8>(isDead));
    }

    GLOBED_DECODE {
        timestamp = buf.readF32();
        percentage = buf.readU16();
        attempts = buf.readI32();

        player1 = buf.readValue<SpecificIconData>();
        player2 = buf.readValue<SpecificIconData>();

        lastDeathTimestamp = buf.readF32();

        buf.readBits<8>().readBitsInto(isDead, isPaused, isPracticing);
    }

    float timestamp;
    uint16_t percentage;
    int32_t attempts;

    SpecificIconData player1;
    SpecificIconData player2;

    float lastDeathTimestamp;

    bool isDead;
    bool isPaused;
    bool isPracticing;
};