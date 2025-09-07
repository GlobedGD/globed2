#pragma once
#include <ui/BasePopup.hpp>
#include <globed/core/net/MessageListener.hpp>
#include <globed/core/data/Messages.hpp>

#include <Geode/Geode.hpp>
#include <cue/ListNode.hpp>

namespace globed {

class CreditsPopup : public BasePopup<CreditsPopup> {
public:
    static const cocos2d::CCSize POPUP_SIZE;

protected:
    cue::ListNode* m_list;
    std::optional<MessageListener<msg::CreditsMessage>> m_listener; // TODO

    bool setup() override;
    void onLoaded(const std::vector<CreditsCategory>& categories);
};

}