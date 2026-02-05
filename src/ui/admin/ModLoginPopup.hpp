#pragma once

#include <ui/BasePopup.hpp>
#include <globed/core/data/Messages.hpp>
#include <globed/core/net/MessageListener.hpp>
#include <ui/misc/LoadingPopup.hpp>

#include <Geode/utils/function.hpp>

namespace globed {

class ModLoginPopup : public BasePopup {
public:
    static ModLoginPopup* create(geode::Function<void()> callback);

protected:
    geode::Function<void()> m_callback;
    geode::TextInput* m_passwordInput = nullptr;
    std::optional<MessageListener<msg::AdminResultMessage>> m_listener;
    LoadingPopup* m_loadPopup = nullptr;

    bool init(geode::Function<void()> callback);
    void wait();
    void stopWaiting(bool success);
};

}
