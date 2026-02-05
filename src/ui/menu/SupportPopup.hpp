#pragma once
#include <ui/BasePopup.hpp>

#include <Geode/Geode.hpp>

namespace globed {

class SupportPopup : public BasePopup {
public:
    static SupportPopup* create();

protected:
    cocos2d::CCSprite* m_background;
    cocos2d::CCSprite* m_ground;
    cocos2d::CCParticleSystemQuad* m_particles;

    bool init() override;
    void update(float dt) override;
    void kofiEnableParticlesCallback();
    void kofiEnableParticlesCallback2(float dt);
};

}