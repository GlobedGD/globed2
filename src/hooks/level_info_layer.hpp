#pragma once
#include <defs/geode.hpp>

#include <Geode/modify/LevelInfoLayer.hpp>

class $modify(HookedLevelInfoLayer, LevelInfoLayer) {
    struct Fields {
        bool allowOpeningAnyway = false;
    };

    void onPlay(cocos2d::CCObject*);
    void forcePlay(cocos2d::CCObject*);
    void tryCloneLevel(cocos2d::CCObject*);
};
