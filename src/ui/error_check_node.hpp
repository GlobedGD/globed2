#pragma once
#include <defs.hpp>

class ErrorCheckNode : public cocos2d::CCNode {
public:
    static ErrorCheckNode* create();

protected:
    bool init() override;
    void updateErrors(float dt);
};