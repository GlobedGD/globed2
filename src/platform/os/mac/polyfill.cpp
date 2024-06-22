#include <platform/basic.hpp>

void cocos2d::CCScheduler::scheduleSelector(SEL_SCHEDULE pfnSelector, CCObject *pTarget, float fInterval, bool bPaused) {
    this->scheduleSelector(pfnSelector, pTarget, fInterval, kCCRepeatForever, 0.f, bPaused);
}
