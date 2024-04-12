#pragma once
#include <defs/geode.hpp>

class GlobedSimplePlayer : public cocos2d::CCNode {
public:
    struct Icons {
        IconType type;
        int id;
        int color1, color2, color3; // color3 is -1 for glow disabled
    };

    static GlobedSimplePlayer* create(const Icons& icons);
    void updateIcons(const Icons& icons);

protected:
    bool init(const Icons& icons);

    Icons icons;
    SimplePlayer* sp;
};
