#pragma once
#include <defs/geode.hpp>

class GlobedCreditsCell : public cocos2d::CCNode {
public:
    static GlobedCreditsCell* create(const char* name, bool lightBg, cocos2d::CCArray* players);

protected:
    bool init(const char* name, bool lightBg, cocos2d::CCArray* players);
};