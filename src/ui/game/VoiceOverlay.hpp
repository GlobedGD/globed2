#pragma once

#include <globed/prelude.hpp>
#include <Geode/Geode.hpp>

namespace globed {

class VoiceOverlay : public cocos2d::CCNode {
public:
    static VoiceOverlay* create();

    void update(float) override;

private:
    bool init() override;
};

}