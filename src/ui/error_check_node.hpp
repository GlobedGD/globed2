#pragma once
#include <cocos2d.h>

class ErrorCheckNode : public cocos2d::CCNode {
public:
    static ErrorCheckNode* create();

protected:
    static bool canShowFLAlert();

    bool init() override;
    void updateErrors(float dt);
};