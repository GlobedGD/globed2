#pragma once
#include <defs/geode.hpp>

#include <Geode/modify/PlayerObject.hpp>

#include <managers/hook.hpp>

class ComplexVisualPlayer;

constexpr int COMPLEX_PLAYER_OBJECT_TAG = 3458738;

struct GLOBED_DLL ComplexPlayerObject : geode::Modify<ComplexPlayerObject, PlayerObject> {
    static void onModify(auto& self) {
        (void) self.setHookPriority("PlayerObject::incrementJumps", -1000000);
        (void) self.setHookPriority("PlayerObject::playDeathEffect", -10);

        GLOBED_MANAGE_HOOK(Gameplay, PlayerObject::incrementJumps);
        GLOBED_MANAGE_HOOK(Gameplay, PlayerObject::playDeathEffect);
    }

    // those are needed so that our changes don't impact actual PlayerObject instances
    bool vanilla();

    // link this `PlayerObject` to a `ComplexVisualPlayer` instance
    void setRemotePlayer(ComplexVisualPlayer* rp);

    $override
    void incrementJumps();

    $override
    void playDeathEffect();
};

// Unlike `ComplexPlayerObject`, this one is made specifically for vanilla player objects, so it is a separate $modify class.
struct HookedPlayerObject : geode::Modify<HookedPlayerObject, PlayerObject> {
    struct Fields {
        bool forcedPlatFlag = false;
    };

    static void onModify(auto& self) {
        GLOBED_MANAGE_HOOK(Gameplay, PlayerObject::playSpiderDashEffect);
        GLOBED_MANAGE_HOOK(Gameplay, PlayerObject::incrementJumps);
        GLOBED_MANAGE_HOOK(Gameplay, PlayerObject::update);
    }

    void cleanupObjectLayer();

    $override
    void playSpiderDashEffect(cocos2d::CCPoint from, cocos2d::CCPoint to);

    $override
    void incrementJumps();

    $override
    void update(float dt);
};
