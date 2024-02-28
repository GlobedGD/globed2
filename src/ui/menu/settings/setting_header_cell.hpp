#pragma once
#include <defs/all.hpp>

class GlobedSettingHeaderCell : public cocos2d::CCLayer {
public:
    static constexpr float CELL_WIDTH = 358.0f;
    static constexpr float CELL_HEIGHT = 45.0f;

    static GlobedSettingHeaderCell* create(const char* name);

private:
    cocos2d::CCLabelBMFont* label;

    bool init(const char* name);
};