#pragma once

#include <ui/BasePopup.hpp>
#include <globed/core/RoomManager.hpp>
#include <globed/util/CStr.hpp>

#include <Geode/Geode.hpp>
#include <cue/ListNode.hpp>

namespace globed {

class RoomSettingsPopup : public BasePopup {
public:
    static const cocos2d::CCSize POPUP_SIZE;
    static const cocos2d::CCSize LIST_SIZE;
    static RoomSettingsPopup* create();

protected:
    cue::ListNode* m_list;

    bool init() override;

    CCNode* makeCell(
        CStr name,
        CStr desc,
        bool RoomSettings::* bptr,
        std::vector<bool RoomSettings::*> incompats = {},
        bool invert = false
    );

    void reloadCheckboxes();
};

}