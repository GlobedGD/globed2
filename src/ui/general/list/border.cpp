#include "border.hpp"

using Type = GlobedListBorder::Type;

using namespace geode::prelude;

bool GlobedListBorder::init(Type type, float listWidth, float listHeight, cocos2d::ccColor4B bgColor) {
    if (!CCNode::init()) return false;

    CCScale9Sprite* top = nullptr;
    CCScale9Sprite* bottom = nullptr; // me
    CCSprite* left = nullptr;
    CCSprite* right = nullptr;

    switch (type) {
        case Type::GJCommentListLayer:
            top = CCScale9Sprite::createWithSpriteFrameName("GJ_commentTop_001.png", {0, 0, 240, 20});
            bottom = CCScale9Sprite::createWithSpriteFrameName("GJ_commentTop_001.png", {0, 0, 240, 20});
            left = CCSprite::createWithSpriteFrameName("GJ_commentSide_001.png");
            right = CCSprite::createWithSpriteFrameName("GJ_commentSide_001.png");
            break;
        case Type::GJCommentListLayerBlue:
            top = CCScale9Sprite::createWithSpriteFrameName("GJ_commentTop2_001.png", {0, 0, 240, 20});
            bottom = CCScale9Sprite::createWithSpriteFrameName("GJ_commentTop2_001.png", {0, 0, 240, 20});
            left = CCSprite::createWithSpriteFrameName("GJ_commentSide2_001.png");
            right = CCSprite::createWithSpriteFrameName("GJ_commentSide2_001.png");
            break;
        default:
            break;
    }

    const float pad = 5.f;
    float width = listWidth + pad;
    float height = listHeight + pad;

    if (top && bottom) {
        top->setContentWidth(width);
        top->setAnchorPoint({0.5f, 1.f});
        top->setPosition(width / 2.f, height);
        this->addChild(top);

        bottom->setContentWidth(width);
        bottom->setAnchorPoint({0.5f, 0.f});
        bottom->setPosition(width / 2.f, 0.f);
        this->addChild(bottom);
    }

    if (left && right) {
        left->setScaleY((listHeight - 30.f) / left->getScaledContentHeight());
        left->setAnchorPoint({0.f, 0.5f});
        left->setPosition(CCPoint{0.f, height / 2.f});
        this->addChild(left);

        right->setScaleY((listHeight - 30.f) / right->getScaledContentHeight());
        right->setAnchorPoint({1.f, 0.5f});
        right->setPosition(CCPoint{width, height / 2.f});
        this->addChild(right);
    }

    this->setContentSize(CCSize{width, height});

    return true;
}