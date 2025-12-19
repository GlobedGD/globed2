#pragma once

#include <ui/BasePopup.hpp>

#include <cue/ListNode.hpp>
#include <cue/Slider.hpp>
#include <asp/time/Instant.hpp>

namespace globed {

class UserListPopup : public BasePopup<UserListPopup> {
public:
    static const cocos2d::CCSize POPUP_SIZE;

    void hardRefresh();

private:
    cue::ListNode* m_list;
    cue::Slider* m_volumeSlider;
    std::string m_searchFilter;
    asp::time::Instant m_lastKeystroke;
    bool m_needsFilterUpdate = false;

    bool setup() override;
    void update(float dt) override;
    void onVolumeChanged(double value);
};

}