#pragma once
#include <defs/all.hpp>

#include <data/packets/client/admin.hpp>

class AdminSendNoticePopup : public geode::Popup<const std::string_view> {
public:
    static constexpr float POPUP_WIDTH = 320.f;
    static constexpr float POPUP_HEIGHT = 160.f;

    static AdminSendNoticePopup* create(const std::string_view message);

private:
    std::string message;
    geode::InputNode *userInput, *roomInput, *levelInput;

    bool setup(const std::string_view message);
    void commonSend(AdminSendNoticeType type);
};
