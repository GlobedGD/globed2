#pragma once
#include <Geode/Geode.hpp>

class ErrorCheckNode : public cocos2d::CCNode {
public:
    static ErrorCheckNode* create();

protected:
    bool init();
    void updateErrors(float _unused);
};