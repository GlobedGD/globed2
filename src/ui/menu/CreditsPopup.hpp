#pragma once
#include <ui/BasePopup.hpp>
#include <globed/core/net/MessageListener.hpp>
#include <globed/core/data/Messages.hpp>

#include <Geode/Geode.hpp>
#include <cue/ListNode.hpp>

namespace globed {

class CreditsPopup : public BasePopup {
public:
    static CreditsPopup* create();

protected:
    cue::ListNode* m_list;
    MessageListener<msg::CreditsMessage> m_listener;

    bool init() override;
    void onLoaded(const std::vector<CreditsCategory>& categories);
};

}