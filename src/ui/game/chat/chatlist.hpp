#pragma once
#include <defs/all.hpp>
#include <Geode/ui/TextInput.hpp>
#include "chat_cell.hpp"

class GlobedChatListPopup : public geode::Popup<> {
protected:
    static constexpr float POPUP_WIDTH = 342.f;
    static constexpr float POPUP_HEIGHT = 240.f;

	bool setup() override;

    CCMenuItemSpriteExtra* reviewButton;
    cocos2d::extension::CCScale9Sprite* background;
    geode::ScrollLayer* scroll = nullptr;
    cocos2d::CCLayer* layer2;
    geode::TextInput* inp;
    cocos2d::CCMenu* menu;
    std::vector<GlobedChatCell*> messageCells;

    float nextY = 0.f;
    int messages = 0;

    void onChat(cocos2d::CCObject* sender);
    void onClose(cocos2d::CCObject* sender) override;

    void updateChat(float dt);

    virtual void keyBackClicked() override;

public:
	static GlobedChatListPopup* create();
    void createMessage(int accountID, const std::string& message);
};
