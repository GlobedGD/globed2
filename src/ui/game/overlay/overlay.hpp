#pragma once
#include <defs.hpp>

class GlobedOverlay : public cocos2d::CCNode {
public:
    bool init();

    void updatePing(uint32_t ms);

    static GlobedOverlay* create();

private:
    cocos2d::CCLabelBMFont* pingLabel;
};
