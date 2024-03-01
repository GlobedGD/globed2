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

struct SpiderTeleportData {
    cocos2d::CCPoint from, to;

    GLOBED_ENCODE {
        buf.writePoint(from);
        buf.writePoint(to);
    }

    GLOBED_DECODE {
        from = buf.readPoint();
        to = buf.readPoint();
    }
};

struct SpecificIconData {
    GLOBED_ENCODE {
        buf.writePoint(position);
        buf.writeF32(rotation);

        buf.writeEnum(iconType);
        buf.writeBits(BitBuffer<16>(
            isVisible,
            isLookingLeft,
            isUpsideDown,
            isDashing,
            isMini,
            isGrounded,
            isStationary,
            isFalling,
            didJustJump
        ));

        buf.writeOptionalValue(spiderTeleportData);
    }

    GLOBED_DECODE {
        position = buf.readPoint();
        rotation = buf.readF32();

        iconType = buf.readEnum<PlayerIconType>();
        GLOBED_REQUIRE(iconType >= PlayerIconType::Unknown && iconType <= PlayerIconType::Swing, "invalid PlayerIconType value encountered when decoding SpecificIconData")
        if (iconType == PlayerIconType::Unknown) {
            iconType = PlayerIconType::Cube;
        }

        buf.readBits<16>().readBitsInto(
            isVisible,
            isLookingLeft,
            isUpsideDown,
            isDashing,
            isMini,
            isGrounded,
            isStationary,
            isFalling,
            didJustJump
        );

        spiderTeleportData = buf.readOptionalValue<SpiderTeleportData>();
    }

    void copyFlagsFrom(const SpecificIconData& other) {
        iconType = other.iconType;
        isDashing = other.isDashing;
        isLookingLeft = other.isLookingLeft;
        isUpsideDown = other.isUpsideDown;
        isVisible = other.isVisible;
        isMini = other.isMini;
        isGrounded = other.isGrounded;
        isStationary = other.isStationary;
        isFalling = other.isFalling;
    }

    cocos2d::CCPoint position;
    float rotation;

    PlayerIconType iconType;
    bool isVisible;         // self-explanatory
    bool isLookingLeft;     // true if player is going to the left side (in platformer)
    bool isUpsideDown;      // true if player is upside down
    bool isDashing;         // true if player is dashing
    bool isMini;            // true if player is mini
    bool isGrounded;        // true if player is on the ground currently
    bool isStationary;      // true if player is not moving currently (in platformer)
    bool isFalling;         // when !isGrounded, true if falling, false if jumping upwards

    bool didJustJump;       // true on the next frame after the player performed a jump
    std::optional<SpiderTeleportData> spiderTeleportData;
};

struct PlayerData {
    GLOBED_ENCODE {
        buf.writeF32(timestamp);

        buf.writeValue(player1);
        buf.writeValue(player2);

        buf.writeF32(lastDeathTimestamp);

        buf.writeF32(currentPercentage);

        buf.writeBits(BitBuffer<8>(isDead, isPaused, isPracticing, isDualMode));
    }

    GLOBED_DECODE {
        timestamp = buf.readF32();

        player1 = buf.readValue<SpecificIconData>();
        player2 = buf.readValue<SpecificIconData>();

        lastDeathTimestamp = buf.readF32();

        currentPercentage = buf.readF32();

        buf.readBits<8>().readBitsInto(isDead, isPaused, isPracticing, isDualMode);
    }

    float timestamp;

    SpecificIconData player1;
    SpecificIconData player2;

    float lastDeathTimestamp;

    float currentPercentage;

    bool isDead;
    bool isPaused;
    bool isPracticing;
    bool isDualMode;
};

struct PlayerMetadata {
    GLOBED_ENCODE {
        buf.writeU32(localBest);
        buf.writeI32(attempts);
    }

    GLOBED_DECODE {
        localBest = buf.readU32();
        attempts = buf.readI32();
    }
    uint32_t localBest;
    int32_t attempts;
};
