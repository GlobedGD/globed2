#pragma once

#include <defs/geode.hpp>

class RelayTipCell : public cocos2d::CCLayer {
public:
    static constexpr float CELL_WIDTH = 358.0f;
    static constexpr float CELL_HEIGHT = 32.0f;
    static RelayTipCell* create();

private:
    bool init();
};
