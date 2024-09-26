#pragma once
#include <defs/geode.hpp>

#include <Geode/modify/PauseLayer.hpp>

#include <managers/hook.hpp>

// Defines globed-related PauseLayer hooks
struct GLOBED_DLL GlobedPauseLayer : geode::Modify<GlobedPauseLayer, PauseLayer> {
    static void onModify(auto& self) {
        (void) self.setHookPriority("PauseLayer::keyDown", -999999999);
        (void) self.setHookPriority("PauseLayer::goEdit", -999999999);

        GLOBED_MANAGE_HOOK(Gameplay, PauseLayer::customSetup);
        GLOBED_MANAGE_HOOK(Gameplay, PauseLayer::goEdit);
        GLOBED_MANAGE_HOOK(Gameplay, PauseLayer::onQuit);
        GLOBED_MANAGE_HOOK(Gameplay, PauseLayer::onResume);
        GLOBED_MANAGE_HOOK(Gameplay, PauseLayer::onRestart);
        GLOBED_MANAGE_HOOK(Gameplay, PauseLayer::onRestartFull);
        GLOBED_MANAGE_HOOK(Gameplay, PauseLayer::onEdit);
        GLOBED_MANAGE_HOOK(Gameplay, PauseLayer::onNormalMode);
        GLOBED_MANAGE_HOOK(Gameplay, PauseLayer::onPracticeMode);
    }

    bool hasPopup();

    $override
    void customSetup();

    $override
    void goEdit();

    void selUpdate(float dt);

    // callbacks

    $override
    void onQuit(cocos2d::CCObject*);

    $override
    void onResume(cocos2d::CCObject*);

    $override
    void onRestart(cocos2d::CCObject*);

    $override
    void onRestartFull(cocos2d::CCObject*);

    $override
    void onEdit(cocos2d::CCObject*);

    $override
    void onNormalMode(cocos2d::CCObject*);

    $override
    void onPracticeMode(cocos2d::CCObject*);
};
