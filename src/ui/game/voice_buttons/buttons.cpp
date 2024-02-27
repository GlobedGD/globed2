#include "buttons.hpp"

using namespace geode::prelude;

bool VoiceChatButtonsMenu::init() {
    if (!CCNode::init()) return false;

    Build<CCMenu>::create()
        .parent(this)
        .store(btnMenu);

    // create the buttons
    // TODO sometime later


    Build<CCScale9Sprite>::create("GJ_button_05.png")
        .contentSize(1.f, 1.f);

    return true;
}

VoiceChatButtonsMenu* VoiceChatButtonsMenu::create() {
    auto ret = new VoiceChatButtonsMenu;
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}