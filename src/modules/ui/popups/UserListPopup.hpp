#pragma once

#include <ui/BasePopup.hpp>

#include <cue/ListNode.hpp>
#include <cue/Slider.hpp>

namespace globed {

class UserListPopup : public BasePopup<UserListPopup> {
public:
    static const cocos2d::CCSize POPUP_SIZE;

    void hardRefresh();

private:
    cue::ListNode* m_list;
    cue::Slider* m_volumeSlider;

    bool setup() override;
    void onVolumeChanged(double value);
};

}