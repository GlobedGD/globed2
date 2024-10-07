#pragma once

#include <cocos2d.h>

class BetterLoadingCircle : public cocos2d::CCSprite {
public:
    // Creates the loading circle, by default is invisible unless `fadeIn` is called or `true` is passed to `create`.
    static BetterLoadingCircle* create(bool spinByDefault = false);

    void fadeOut();
    void fadeIn();

private:
    bool init(bool spinByDefault);
    void deactivate();
};