#pragma once
#include <Geode/Geode.hpp>

class ErrorCheckNode : public cocos2d::CCNode {
public:
    bool init();
    void updateErrors(float _unused);

    static ErrorCheckNode* create() {
        auto* ret = new ErrorCheckNode;
        if (ret && ret->init()) {
            ret->autorelease();
            return ret;
        }

        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};