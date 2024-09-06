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

GLOBED_SERIALIZABLE_ENUM(PlayerIconType, Unknown, Cube, Ship, Ball, Ufo, Wave, Robot, Spider, Swing, Jetpack);

struct SpiderTeleportData {
    cocos2d::CCPoint from, to;
};

GLOBED_SERIALIZABLE_STRUCT(SpiderTeleportData, (from, to));


struct GlobedCounterChange {
    enum class Type : uint8_t {
        Set = 0, Add = 1, Multiply = 2, Divide = 3
    };

    int itemId;
    Type type;
    union { int intVal; float floatVal; } _val;
};

GLOBED_SERIALIZABLE_ENUM(GlobedCounterChange::Type, Set, Add, Multiply, Divide);

struct SpecificIconData {
    void copyFlagsFrom(const SpecificIconData& other);

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
    bool isRotating;        // true if player is, well, rotating
    bool isSideways;        // true if player is stuck to a wall
    std::optional<SpiderTeleportData> spiderTeleportData;
};

struct PlayerData {
    float timestamp;

    SpecificIconData player1;
    SpecificIconData player2;

    float lastDeathTimestamp;

    float currentPercentage;

    bool isDead;
    bool isPaused;
    bool isPracticing;
    bool isDualMode;
    bool isInEditor;
    bool isEditorBuilding; // in the editor && not playtesting (incl. not paused)
    bool isLastDeathReal; // for deathlink, to prevent death chains

    std::vector<GlobedCounterChange> counterChanges;
};

struct PlayerMetadata {
    uint32_t localBest;
    int32_t attempts;
};

GLOBED_SERIALIZABLE_STRUCT(PlayerMetadata, (
    localBest,
    attempts
));
