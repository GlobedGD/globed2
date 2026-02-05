#pragma once

#include <ui/BasePopup.hpp>
#include <Geode/utils/function.hpp>

namespace globed {

class DiscordLinkAttemptPopup : public BasePopup {
public:
    static DiscordLinkAttemptPopup* create(uint64_t userId, const std::string& username, const std::string& avatarUrl);
    using Callback = geode::Function<void(bool)>;

    void setCallback(Callback&& cb);

private:
    uint64_t m_userId;
    Callback m_callback;

    bool init(uint64_t, const std::string&, const std::string&);
    void confirm(bool accept);
};

// Converts a URL to use .png if image plus is not installed
std::string convertAvatarUrl(const std::string& url);

}