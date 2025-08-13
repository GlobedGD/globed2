#pragma once
#include <Geode/Geode.hpp>

namespace globed {

class UnreadBadge : public cocos2d::CCNode {
public:
    static UnreadBadge* create(int count);

private:
    bool init(int count);
};

}