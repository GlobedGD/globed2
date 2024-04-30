#pragma once
#include <defs/all.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/ui/TextInput.hpp>

using namespace geode::prelude;

class GlobedChatListPopup : public geode::Popup<> {
protected:
	bool setup() override;

    CCMenuItemSpriteExtra* reviewButton;
    CCScale9Sprite* background;
    ScrollLayer* scroll = nullptr;
    CCLayer* layer2;
    geode::TextInput* inp;
    CCMenu* menu;

    float nextY = 113.f;

    void onChat(CCObject* sender);
    void onClose(CCObject* sender) override;

    virtual void keyBackClicked() override;

public:
	static GlobedChatListPopup* create();
    void getChat();
};