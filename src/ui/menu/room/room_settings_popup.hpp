#pragma once

#include <defs/geode.hpp>
#include <managers/room.hpp>

class RoomSettingCell;

class RoomSettingsPopup : public geode::Popup<> {
public:
    static constexpr float POPUP_WIDTH = 250.f;
    static constexpr float POPUP_HEIGHT = 170.f;
    static constexpr float LIST_WIDTH = 340.f;
    static constexpr float LIST_HEIGHT = 220.f;

    static RoomSettingsPopup* create();

    void onSettingClicked(cocos2d::CCObject* sender);
    void updateCheckboxes();

    void enableCheckboxes(bool enabled);

private:
    friend class RoomSettingCell;

    RoomSettings currentSettings = {};
    RoomSettingCell
        *cellInviteOnly,
        *cellCollision,
        *cellTwoPlayer,
        *cellPublicInvites
        ;

    bool setup() override;
};

class RoomSettingCell : public cocos2d::CCLayer {
public:
    static constexpr float CELL_HEIGHT = 45.0f;
    static constexpr float CELL_WIDTH = RoomSettingsPopup::LIST_WIDTH;

    static RoomSettingCell* create(const char* name, std::string desc, int tag, RoomSettingsPopup* popup);

private:
    friend class RoomSettingsPopup;

    RoomSettingsPopup* popup;
    Ref<CCMenuItemToggler> button;

    bool init(const char* name, std::string desc, int tag, RoomSettingsPopup* popup);

    void setToggled(bool state);
    void setEnabled(bool state);
};