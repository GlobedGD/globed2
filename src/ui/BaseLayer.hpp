#pragma once

#include <Geode/Geode.hpp>

namespace globed {

class BaseLayer : public cocos2d::CCLayer {
public:
    virtual void keyBackClicked();
    virtual void switchTo();

protected:
    cocos2d::CCMenu* m_backMenu = nullptr;
    CCMenuItemSpriteExtra* m_backButton = nullptr;
    cocos2d::CCSprite* m_background = nullptr;

    bool init(bool background = true);
    void initBackground(cocos2d::ccColor3B color = {0, 102, 255});
};

}