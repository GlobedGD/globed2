#pragma once
#include <defs/geode.hpp>

#include <Geode/modify/PauseLayer.hpp>

class $modify(GlobedPauseLayer, PauseLayer) {
    static void onModify(auto& self) {
        (void) self.setHookPriority("PauseLayer::keyDown", -999999999);
    }

    $override
    void customSetup();

    $override
    void goEdit();

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
