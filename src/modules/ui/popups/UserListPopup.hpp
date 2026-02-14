#pragma once

#include <ui/BasePopup.hpp>

#include <cue/ListNode.hpp>
#include <cue/Slider.hpp>
#include <asp/time/Instant.hpp>
#include <AdvancedLabel.hpp>

namespace globed {

class UserListPopup : public BasePopup {
public:
    static UserListPopup* create();

    void hardRefresh();

private:
    cue::ListNode* m_list;
    cue::Slider* m_volumeSlider;
    std::string m_searchFilter;
    CCNode* m_serverContainer = nullptr;
    Label* m_serverNameLabel = nullptr;
    Label* m_serverPingLabel = nullptr;
    asp::time::Instant m_lastKeystroke;
    bool m_needsFilterUpdate = false;

    bool init() override;
    void update(float dt) override;
    void updateServer(float dt);
    void onVolumeChanged(double value);
};

}