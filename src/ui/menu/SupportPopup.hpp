#pragma once
#include <ui/BasePopup.hpp>

#include <Geode/Geode.hpp>

namespace globed {

class SupportPopup : public BasePopup<SupportPopup, cocos2d::CCSprite *> {
public:
    static cocos2d::CCSize POPUP_SIZE;

    static SupportPopup *create();

protected:
    cocos2d::CCSprite *m_background;
    cocos2d::CCSprite *m_ground;
    cocos2d::CCParticleSystemQuad *m_particles;

    bool setup(cocos2d::CCSprite *) override;
    void update(float dt) override;
    void kofiEnableParticlesCallback();
    void kofiEnableParticlesCallback2(float dt);
};

} // namespace globed