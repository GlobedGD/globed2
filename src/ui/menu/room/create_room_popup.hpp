#pragma once
#include <defs/all.hpp>

#include <data/types/room.hpp>

class RoomLayer;
namespace geode { class TextInput; }

class CreateRoomPopup : public geode::Popup<RoomLayer*> {
public:
    constexpr static float POPUP_WIDTH = 340.f;
    constexpr static float POPUP_HEIGHT = 200.f;

    static CreateRoomPopup* create(RoomLayer* parent);

protected:
    geode::TextInput* roomNameInput;
    geode::TextInput* passwordInput;
    geode::TextInput* playerLimitInput;
    RoomSettingsFlags settingFlags = {};

    bool setup(RoomLayer* parent) override;
    void onCheckboxToggled(cocos2d::CCObject*);
};