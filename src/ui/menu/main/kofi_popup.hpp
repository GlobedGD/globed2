#pragma once

#include <defs/geode.hpp>

class GlobedKofiPopup : public geode::Popup<cocos2d::CCSprite*> {
public:
    static GlobedKofiPopup* create();

private:
    bool setup(cocos2d::CCSprite*);
};
