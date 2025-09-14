#pragma once

#include <ui/BasePopup.hpp>

#include <cue/ListNode.hpp>

namespace globed {

class KeybindsPopup : public BasePopup<KeybindsPopup> {
public:
    static const cocos2d::CCSize POPUP_SIZE;

protected:
    cue::ListNode* m_list;

    bool setup() override;
};

}