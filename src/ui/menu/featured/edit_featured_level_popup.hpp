#pragma once

#include <defs/geode.hpp>
#include <asp/math/NumberCycle.hpp>

#include <managers/web.hpp>

class EditFeaturedLevelPopup : public geode::Popup<GJGameLevel*> {
public:
    static constexpr float POPUP_WIDTH = 340.f;
    static constexpr float POPUP_HEIGHT = 190.f;

    static EditFeaturedLevelPopup* create(GJGameLevel* level);

private:
    bool setup(GJGameLevel*) override;
    void save();
    int getDifficulty();
    void createDiffButton();
    void onDiffClick(CCObject* sender);

    void onRequestComplete(typename WebRequestManager::Event* event);

    geode::TextInput* notesInput;
    Ref<CCMenuItemSpriteExtra> featureButton;
    Ref<CCMenuItemSpriteExtra> sendButton;
    Ref<CCMenuItemSpriteExtra> curDiffButton;
    asp::NumberCycle currIdx{0, 2};

    Ref<GJGameLevel> level;

    Ref<cocos2d::CCMenu> menu;

    WebRequestManager::Listener reqListener;
};
