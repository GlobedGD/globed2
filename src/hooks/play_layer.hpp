#pragma once
#include <defs/geode.hpp>

#include <Geode/modify/PlayLayer.hpp>
#include <optional>

#include <managers/hook.hpp>

struct GLOBED_DLL GlobedPlayLayer : geode::Modify<GlobedPlayLayer, PlayLayer> {
    struct Fields {
        GameObject* antiCheat = nullptr;
        bool ignoreNoclip = false;
        std::optional<bool> oldShowProgressBar;
    };

    static void onModify(auto& self) {
        (void) self.setHookPriority("PlayLayer::resetLevel", 99999999);
        (void) self.setHookPriority("PlayLayer::destroyPlayer", -99999999);

        GLOBED_MANAGE_HOOK(Gameplay, PlayLayer::resetLevel);
        GLOBED_MANAGE_HOOK(Gameplay, PlayLayer::destroyPlayer);
        GLOBED_MANAGE_HOOK(Gameplay, PlayLayer::onQuit);
        GLOBED_MANAGE_HOOK(Gameplay, PlayLayer::fullReset);
        GLOBED_MANAGE_HOOK(Gameplay, PlayLayer::showNewBest);
        GLOBED_MANAGE_HOOK(Gameplay, PlayLayer::levelComplete);
    }

    // gd hooks

    $override
    bool init(GJGameLevel* level, bool p1, bool p2);

    $override
    void setupHasCompleted();

    $override
    void onQuit();

    $override
    void fullReset();

    $override
    void resetLevel();

    $override
    void showNewBest(bool p0, int p1, int p2, bool p3, bool p4, bool p5);

    $override
    void levelComplete();

    $override
    void destroyPlayer(PlayerObject* p0, GameObject* p1);

    void forceKill(PlayerObject* p);

    Fields& getFields();
};
