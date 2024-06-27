#pragma once
#include <defs/geode.hpp>

class GlobedContentLayer : geode::GenericContentLayer {
    void setPosition(const cocos2d::CCPoint& pos) override;
};
