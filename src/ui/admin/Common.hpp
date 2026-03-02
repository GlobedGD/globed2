#pragma once

#include <globed/core/net/MessageListener.hpp>
#include <globed/core/data/Messages.hpp>
#include <core/net/NetworkManagerImpl.hpp>
#include <ui/misc/LoadingPopup.hpp>
#include <Geode/utils/function.hpp>

namespace globed {

inline void waitForAdminResult(geode::Function<void(geode::Result<>)> callback = {}) {
    using Callback = decltype(callback);

    struct WaitData : public cocos2d::CCObject {
        Callback m_callback;
        MessageListener<msg::AdminResultMessage> m_listener;
    };

    auto lp = LoadingPopup::create();
    lp->setClosable(true);
    lp->show();

    auto wd = new WaitData();
    wd->autorelease();
    lp->setUserObject("callback"_spr, wd);

    auto listener = NetworkManagerImpl::get().listen<msg::AdminResultMessage>([lp, wd](const msg::AdminResultMessage& msg) {
        lp->forceClose();

        if (wd->m_callback) {
            msg.success ? wd->m_callback(Ok()) : wd->m_callback(Err(msg.error));
        }

        if (msg.success) {
            globed::toastSuccess("Success");
        } else {
            globed::alertFormat(
                "Error",
                "Action failed due to the following error:\n\n<cy>{}</c>",
                msg.error
            );
        }

        return ListenerResult::Stop;
    });
    listener.setPriority(-100);
    wd->m_listener = std::move(listener);
}

}
