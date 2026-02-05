#pragma once

#include <globed/core/net/MessageListener.hpp>
#include <globed/core/data/Messages.hpp>
#include <ui/BasePopup.hpp>

#include <cue/ListNode.hpp>

namespace globed {

class UserSettingsPopup : public BasePopup {
public:
    static UserSettingsPopup* create();

protected:
    CCMenu* m_menu = nullptr;
    bool m_modified = false;

    bool init() override;
    void addButton(std::string_view key, const char* onSprite, const char* offSprite);
    void onClose(CCObject*) override;
};

}