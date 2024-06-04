#pragma once
#include <defs/all.hpp>

class GlobedOverlay : public cocos2d::CCNode {
public:
    bool init();

    void updatePing(uint32_t ms);
    void updateWithDisconnected();
    void updateWithEditor();

    static GlobedOverlay* create();

private:
    cocos2d::CCLabelBMFont
        *pingLabel = nullptr,
        *versionLabel = nullptr;
};
