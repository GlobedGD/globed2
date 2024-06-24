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
            border->setContentSize({listWidth, listHeight});
            border->setPosition({listWidth / 2.f, listHeight / 2.f});
            this->addChild(border);
        } default: break;
    }

    this->setContentSize({listWidth, listHeight});


    // this->setAnchorPoint({.5f, .5f});

    // CCScale9Sprite* top = nullptr;
    // CCScale9Sprite* bottom = nullptr; // me
    // CCSprite* left = nullptr;
    // CCSprite* right = nullptr;

    // switch (type) {
    //     case Type::GJCommentListLayer:
    //         top = CCScale9Sprite::createWithSpriteFrameName("GJ_commentTop_001.png");
    //         bottom = CCScale9Sprite::createWithSpriteFrameName("GJ_commentTop_001.png");
    //         left = CCSprite::createWithSpriteFrameName("GJ_commentSide_001.png");
    //         right = CCSprite::createWithSpriteFrameName("GJ_commentSide_001.png");
    //         break;
    //     case Type::GJCommentListLayerBlue:
    //         top = CCScale9Sprite::createWithSpriteFrameName("GJ_commentTop2_001.png");
    //         bottom = CCScale9Sprite::createWithSpriteFrameName("GJ_commentTop2_001.png");
    //         left = CCSprite::createWithSpriteFrameName("GJ_commentSide2_001.png");
    //         right = CCSprite::createWithSpriteFrameName("GJ_commentSide2_001.png");
    //         break;
    //     default:
    //         break;
    // }

    // const float pad = 10.f;
    // float width = listWidth + pad;
    // float height = listHeight + pad;

    // this->setContentSize(CCSize{width, height});

    // if (top && bottom) {
    //     top->setContentWidth(width);
    //     this->addChildAtPosition(top, Anchor::Top, {0, -top->getContentHeight() / 3.f});

    //     bottom->setContentWidth(width);
    //     bottom->setScaleY(-1.f);
    //     this->addChildAtPosition(bottom, Anchor::Bottom, {0, -top->getContentHeight() / 3.f});
    // }

    // if (left && right) {
    //     left->setScaleY((listHeight - 30.f) / left->getScaledContentHeight());
    //     this->addChildAtPosition(left, Anchor::Left, {0, 0});

    //     right->setScaleY((listHeight - 30.f) / right->getScaledContentHeight());
    //     right->setFlipX(true);
    //     this->addChildAtPosition(right, Anchor::Right, {0, 0});
    // }

    return true;
}