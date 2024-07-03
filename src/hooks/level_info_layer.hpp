#pragma once
#include <defs/geode.hpp>

#include <Geode/modify/LevelInfoLayer.hpp>

class $modify(HookedLevelInfoLayer, LevelInfoLayer) {
    static void onModify(auto& self) {
        (void) self.setHookPriority("LevelInfoLayer::onPlay", -9999);
    }

    struct Fields {
        bool allowOpeningAnyway = false;
        int rateTier = -1;
    };

    void onPlay(cocos2d::CCObject*);
    void forcePlay(cocos2d::CCObject*);
    void tryCloneLevel(cocos2d::CCObject*);

    bool init(GJGameLevel* level, bool challenge);

};
