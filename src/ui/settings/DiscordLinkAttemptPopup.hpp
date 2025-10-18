#pragma once

#include <ui/BasePopup.hpp>

#include <std23/move_only_function.h>

namespace globed {

class DiscordLinkAttemptPopup : public BasePopup<DiscordLinkAttemptPopup, uint64_t, const std::string&, const std::string&> {
public:
    static const cocos2d::CCSize POPUP_SIZE;
    using Callback = std23::move_only_function<void(bool)>;

    void setCallback(Callback&& cb);

private:
    uint64_t m_userId;
    Callback m_callback;

    bool setup(uint64_t, const std::string&, const std::string&) override;
    void confirm(bool accept);
};

// Converts a URL to use .png if image plus is not installed
std::string convertAvatarUrl(const std::string& url);

}