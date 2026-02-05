#pragma once

#include <ui/BasePopup.hpp>

#include <cue/ListNode.hpp>

namespace globed {

class KeybindsPopup : public BasePopup {
public:
    static KeybindsPopup* create();

protected:
    cue::ListNode* m_list;

    bool init() override;
};

}