#pragma once
#include <defs/all.hpp>

#include <data/types/room.hpp>

class RoomPopup;
namespace geode { class TextInput; }

class CreateRoomPopup : public geode::Popup<RoomPopup*> {
public:
    constexpr static float POPUP_WIDTH = 340.f;
    constexpr static float POPUP_HEIGHT = 200.f;

    static CreateRoomPopup* create(RoomPopup* parent);

protected:
    geode::TextInput* roomNameInput;
    geode::TextInput* passwordInput;
    geode::TextInput* playerLimitInput;
    RoomSettingsFlags settingFlags;

    bool setup(RoomPopup* parent) override;
    void onCheckboxToggled(cocos2d::CCObject*);
};