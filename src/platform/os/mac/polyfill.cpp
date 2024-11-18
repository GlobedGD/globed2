#include <platform/basic.hpp>
#include <cocos2d.h>

void cocos2d::CCScheduler::scheduleSelector(SEL_SCHEDULE pfnSelector, CCObject *pTarget, float fInterval, bool bPaused) {
    this->scheduleSelector(pfnSelector, pTarget, fInterval, kCCRepeatForever, 0.f, bPaused);
}

float cocos2d::ccpDistance(const CCPoint &v1, const CCPoint &v2) {
    return v1.getDistance(v2);
}