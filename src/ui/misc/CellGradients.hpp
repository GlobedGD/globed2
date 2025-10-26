#pragma once

#include <globed/prelude.hpp>

namespace globed {

enum class CellGradientType {
    White,
    Friend,
    FriendIngame,
    Self,
    SelfIngame,
    RoomOwner
};

CCSprite* addCellGradient(CCNode* node, CellGradientType type, bool blend = false);

}
