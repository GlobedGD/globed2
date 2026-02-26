#pragma once

#include <ui/BasePopup.hpp>
#include <globed/core/RoomManager.hpp>
#include <globed/util/CStr.hpp>

#include <Geode/Geode.hpp>
#include <cue/ListNode.hpp>

namespace globed {

class RoomSettingsPopup : public BasePopup {
public:
    static RoomSettingsPopup* create(RoomSettings s);
    using Callback = geode::Function<void(RoomSettings)>;

    void setCallback(Callback cb) { m_callback = std::move(cb); }
    void onToggled(bool RoomSettings::* bptr, bool state);

    RoomSettings m_settings;
protected:
    cue::ListNode* m_list = nullptr;
    Callback m_callback;
    CCMenuItemSpriteExtra* m_safeModeBtn;

    enum class RoomSettingKind {
        Room,
        Gamemode,
    };

    struct RoomSetting {
        CStr m_name;
        CStr m_desc;
        bool RoomSettings::* m_ptr;
        RoomSettingKind m_kind;
        asp::SmallVec<bool RoomSettings::*, 8> m_incompats;
        bool m_invert;
        CCMenuItemToggler* m_toggler = nullptr;
    };

    std::vector<RoomSetting> m_cellSetups;

    bool init(RoomSettings s);
    void showSafeModePopup(bool firstTime);
    void reloadCheckboxes();
};

}