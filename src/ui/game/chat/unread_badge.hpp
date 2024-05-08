#pragma once

#include <defs/geode.hpp>

class UnreadMessagesBadge : public cocos2d::CCNode {
public:
    static UnreadMessagesBadge* create(int count);

private:
    bool init(int count);
};
