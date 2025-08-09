#pragma once

#include <ui/BasePopup.hpp>
#include <globed/core/data/Messages.hpp>
#include <globed/core/net/MessageListener.hpp>
#include <ui/misc/LoadingPopup.hpp>

namespace globed {

class ModLoginPopup : public BasePopup<ModLoginPopup, std::function<void()>> {
public:
    static const cocos2d::CCSize POPUP_SIZE;

protected:
    std::function<void()> m_callback;
    geode::TextInput* m_passwordInput = nullptr;
    std::optional<MessageListener<msg::AdminResultMessage>> m_listener;
    LoadingPopup* m_loadPopup = nullptr;

    bool setup(std::function<void()> callback);
    void wait();
    void stopWaiting();
};

}
