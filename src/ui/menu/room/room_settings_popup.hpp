#pragma once

#include <defs/geode.hpp>
#include <managers/room.hpp>

class RoomSettingsPopup : public geode::Popup<> {
public:
    static constexpr float POPUP_WIDTH = 400.f;
    static constexpr float POPUP_HEIGHT = 270.f;

    static RoomSettingsPopup* create();

    void onSettingClicked(cocos2d::CCObject* sender);
    void updateCheckboxes();

    void enableCheckboxes(bool enabled);

private:
    RoomSettings currentSettings;
    Ref<CCMenuItemToggler> btnCollision, btnTwoPlayer;

    bool setup() override;
    ~RoomSettingsPopup();
};