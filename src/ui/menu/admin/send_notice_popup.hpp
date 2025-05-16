#pragma once
#include <defs/all.hpp>

#include <Geode/ui/TextInput.hpp>

#include <data/packets/client/admin.hpp>

class AdminSendNoticePopup : public geode::Popup<std::string_view> {
public:
    static constexpr float POPUP_WIDTH = 320.f;
    static constexpr float POPUP_HEIGHT = 160.f;

    static AdminSendNoticePopup* create(std::string_view message);

private:
    std::string message;
    geode::TextInput *userInput, *roomInput, *levelInput;
    CCMenuItemToggler* userCanReplyCheckbox;
    bool m_alreadyEstimated = false;

    bool setup(std::string_view message);
    void commonSend(AdminSendNoticeType type);
};
