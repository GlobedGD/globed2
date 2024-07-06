#pragma once

#include "Geode/binding/CCMenuItemSpriteExtra.hpp"
#include <defs/geode.hpp>

#include <managers/web.hpp>

class EditFeaturedLevelPopup : public geode::Popup<> {
public:
    static constexpr float POPUP_WIDTH = 340.f;
    static constexpr float POPUP_HEIGHT = 170.f;

    static EditFeaturedLevelPopup* create(GJGameLevel* level);

private:
    bool setup() override;
    void save();
    int getDifficulty();
    void createDiffButton();
    void onDiffClick(CCObject* sender);

    void onRequestComplete(typename WebRequestManager::Event* event);

    geode::TextInput* notesInput;
    Ref<CCMenuItemSpriteExtra> featureButton;
    Ref<CCMenuItemSpriteExtra> sendButton;
    Ref<CCMenuItemSpriteExtra> curDiffButton;
    int currIdx = 0;

    GJGameLevel* level;

    Ref<cocos2d::CCMenu> menu;
    
    WebRequestManager::Listener reqListener;
};
