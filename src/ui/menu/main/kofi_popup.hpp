#pragma once

#include <ui/general/simple_player.hpp>
#include <ui/general/name_label.hpp>
#include <defs/geode.hpp>

class GlobedKofiPopup : public geode::Popup<cocos2d::CCSprite*> {
public:
    static GlobedKofiPopup* create();

private:
    cocos2d::CCSprite* background;
    cocos2d::CCSprite* ground;
    cocos2d::CCParticleSystemQuad* particles;

    bool setup(cocos2d::CCSprite*) override;
    void update(float dt) override;
    void kofiCallback(CCObject* sender);
    void kofiEnableParticlesCallback();
};
