#pragma once
#include <globed/core/data/Messages.hpp>
#include <globed/core/net/MessageListener.hpp>
#include <ui/BasePopup.hpp>

#include <Geode/Geode.hpp>
#include <cue/ListNode.hpp>

namespace globed {

class CreditsPopup : public BasePopup<CreditsPopup> {
public:
    static const cocos2d::CCSize POPUP_SIZE;

protected:
    cue::ListNode *m_list;
    std::optional<MessageListener<msg::CreditsMessage>> m_listener;

    bool setup() override;
    void onLoaded(const std::vector<CreditsCategory> &categories);
};

} // namespace globed