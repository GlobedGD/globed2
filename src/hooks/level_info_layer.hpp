#pragma once
#include <defs/geode.hpp>

#include <Geode/modify/LevelInfoLayer.hpp>

class $modify(HookedLevelInfoLayer, LevelInfoLayer) {
    void onPlay(cocos2d::CCObject*);
    void tryCloneLevel(cocos2d::CCObject*);
};
