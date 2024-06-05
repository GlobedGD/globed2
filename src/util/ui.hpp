#pragma once

#include <defs/all.hpp>
#include <data/types/gd.hpp>
#include <functional>

namespace util::ui {
    constexpr cocos2d::ccColor4B BG_COLOR_BROWN = {191, 114, 62, 255};
    constexpr cocos2d::ccColor4B BG_COLOR_DARKBROWN = {161, 88, 44, 255};
    constexpr cocos2d::ccColor4B BG_COLOR_TRANSPARENT = {0, 0, 0, 180};
    const cocos2d::CCSize BADGE_SIZE = {18.f, 18.f};

    void switchToScene(cocos2d::CCScene* layer);
    void switchToScene(cocos2d::CCLayer* layer);

    void prepareLayer(cocos2d::CCLayer* layer);
    void addBackground(cocos2d::CCNode* layer);
    void addBackButton(cocos2d::CCMenu* menu, std::function<void()> callback);
    void navigateBack();

    // set the scale for `node` in a way that `node->getScaledContentSize()` will match `target->getScaledContentSize()`
    void rescaleToMatch(cocos2d::CCNode* node, cocos2d::CCNode* target, bool stretch = false);

    // set the scale for `node` in a way that `node->getScaledContentSize()` will match `targetSize`
    void rescaleToMatch(cocos2d::CCNode *node, cocos2d::CCSize targetSize, bool stretch = false);

    float getScrollPos(BoomListView* listView);
    void setScrollPos(BoomListView* listView, float pos);

    // scrolls to the bottom of the list view
    void scrollToBottom(BoomListView* listView);
    // scrolls to the bottom of the scroll layer
    void scrollToBottom(geode::ScrollLayer* listView);

    // scrolls to the top of the list view
    void scrollToTop(BoomListView* listView);
    // scrolls to the top of the scroll layer
    void scrollToTop(geode::ScrollLayer* listView);

    void tryLoadDeathEffect(int id);

    // small wrapper with precalculated sizes to make ui easier
    struct PopupLayout {
        cocos2d::CCSize winSize, popupSize;
        float left, right, bottom, top;

        cocos2d::CCSize center, centerLeft, centerTop, centerRight, centerBottom;
        cocos2d::CCSize bottomLeft, topLeft, bottomRight, topRight;
    };

    PopupLayout getPopupLayout(cocos2d::CCSize popupSize);

    cocos2d::CCNode* findChildByMenuSelectorRecursive(cocos2d::CCNode* node, uintptr_t function);

    cocos2d::CCSprite* createBadge(const std::string& sprite);
    cocos2d::CCSprite* createBadgeIfSpecial(const SpecialUserData& data);

    cocos2d::ccColor3B getNameColor(const SpecialUserData& data);
    RichColor getNameRichColor(const SpecialUserData& data);

    void animateLabelColorTint(cocos2d::CCLabelBMFont* label, const RichColor& color);
}
