#include "border.hpp"

using Type = GlobedListBorder::Type;

using namespace geode::prelude;

bool GlobedListBorder::init(Type type, float listWidth, float listHeight, cocos2d::ccColor4B bgColor) {
    if (!CCNode::init()) return false;

    const char* spriteFrameTop = "GJ_commentTop_001.png";
    const char* spriteFrameSide = "GJ_commentSide_001.png";

    switch (type) {
        case Type::GJCommentListLayerBlue:
            spriteFrameTop = "GJ_commentTop2_001.png";
            spriteFrameSide = "GJ_commentSide2_001.png";
            // fallthrough
        case Type::GJCommentListLayer: {
            auto* border = ListBorders::create();
            border->setSpriteFrames(spriteFrameTop, spriteFrameSide);
            border->setContentSize({listWidth, listHeight + 6.f});
            border->setPosition({listWidth / 2.f, listHeight / 2.f});
            this->addChild(border);
        } default: break;
    }

    this->setContentSize({listWidth, listHeight});

    return true;
}