#pragma once

#include <globed/core/data/Messages.hpp>
#include <globed/core/net/MessageListener.hpp>
#include <ui/BasePopup.hpp>

#include <cue/ListNode.hpp>

namespace globed {

class UserSettingsPopup : public BasePopup<UserSettingsPopup> {
public:
    static const CCSize POPUP_SIZE;

protected:
    CCMenu *m_menu = nullptr;
    bool m_modified = false;

    bool setup() override;
    void addButton(std::string_view key, const char *onSprite, const char *offSprite);
    void onClose(CCObject *) override;
};

} // namespace globed