#pragma once

#include <stdint.h>
#include <cocos2d.h>

namespace globed {

enum class PlayerIconType : uint16_t {
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

struct ExtendedPlayerData {
    float velocityX;
    float velocityY;
    bool accelerating;
    float acceleration;
    float fallStartY;
    bool isOnGround2;
    float gravityMod;
    float gravity;
};

struct PlayerObjectData {
    cocos2d::CCPoint position;
    float rotation;
    PlayerIconType iconType;

    bool isVisible;
    bool isLookingLeft;
    bool isUpsideDown;
    bool isDashing;
    bool isMini;
    bool isGrounded;
    bool isStationary;
    bool isFalling;
    bool isRotating;
    bool isSideways;

    std::optional<ExtendedPlayerData> extData;

    /// Copies everything except position and rotation from another `PlayerObjectData`
    void copyFlagsFrom(const PlayerObjectData& other) {
        iconType = other.iconType;
        isVisible = other.isVisible;
        isLookingLeft = other.isLookingLeft;
        isUpsideDown = other.isUpsideDown;
        isDashing = other.isDashing;
        isMini = other.isMini;
        isGrounded = other.isGrounded;
        isStationary = other.isStationary;
        isFalling = other.isFalling;
        isRotating = other.isRotating;
        isSideways = other.isSideways;
    }
};

struct PlayerState {
    int accountId;
    float timestamp;
    uint8_t frameNumber;
    uint8_t deathCount;
    uint16_t percentage;
    bool isDead;
    bool isPaused;
    bool isPracticing;
    bool isInEditor;
    bool isEditorBuilding;
    bool isLastDeathReal;
    std::optional<PlayerObjectData> player1;
    std::optional<PlayerObjectData> player2;

    double progress() const {
        return static_cast<double>(percentage) / 65535.0;
    }
};

}