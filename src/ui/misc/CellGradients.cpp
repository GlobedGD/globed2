#include "CellGradients.hpp"

#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace globed {

static ccColor4B getColor(CellGradientType type) {
    switch (type) {
        case CellGradientType::Friend:
            return ccColor4B{0, 255, 100, 90};
        case CellGradientType::FriendIngame:
            return ccColor4B{255, 255, 255, 75};
        case CellGradientType::Self:
            return ccColor4B{255, 195, 165, 75};
        case CellGradientType::SelfIngame:
            return ccColor4B{0xe8, 0xb7, 0xb7, 150};
        case CellGradientType::RoomOwner:
            return ccColor4B{255, 174, 0, 100};
        case CellGradientType::White:
            return ccColor4B{255, 255, 255, 255};
    }

    return ccColor4B{255, 255, 255, 255};
}

CCSprite* addCellGradient(CCNode* node, CellGradientType type, bool blend) {
    auto color = getColor(type);

    auto gradient = Build<CCSprite>::create("friend-gradient.png"_spr)
        .color({color.r, color.g, color.b})
        .opacity(color.a)
        .pos(0.f, 0.f)
        .anchorPoint({0.f, 0.f})
        .zOrder(-2)
        .parent(node)
        .collect();

    gradient->setScaleX(node->getScaledContentWidth() / gradient->getScaledContentWidth() / 2);
    gradient->setScaleY(node->getScaledContentHeight() / gradient->getScaledContentHeight());

    if (blend) {
        gradient->setBlendFunc({GL_ONE, GL_ONE});
    }

    return gradient;
}

}