#pragma once

#include <globed/core/net/MessageListener.hpp>
#include <globed/core/data/Messages.hpp>
#include <core/net/NetworkManagerImpl.hpp>
#include <ui/misc/LoadingPopup.hpp>
#include <Geode/utils/function.hpp>

namespace globed {

template <typename T>
inline void waitForMessage(geode::Function<void(const T&)> callback = {}) {
    using Callback = decltype(callback);

    struct WaitData : public cocos2d::CCObject {
        Callback m_callback;
        MessageListener<T> m_listener;
    };

    auto lp = LoadingPopup::create();
    lp->setClosable(true);
    lp->show();

    auto wd = new WaitData();
    wd->autorelease();
    wd->m_callback = std::move(callback);
    lp->setUserObject("callback"_spr, wd);

    auto listener = NetworkManagerImpl::get().listen<T>([ref = geode::WeakRef(lp), wd](const T& msg) {
        auto lp = ref.lock();
        if (!lp) return ListenerResult::Continue;

        lp->forceClose();

        if (wd->m_callback) {
            wd->m_callback(msg);
        }

        return ListenerResult::Stop;
    });
    listener.setPriority(-100);
    wd->m_listener = std::move(listener);
}

inline void waitForAdminResult(geode::Function<void(geode::Result<>)> callback = {}) {
    waitForMessage<msg::AdminResultMessage>([callback = std::move(callback)](const msg::AdminResultMessage& msg) mutable {
        if (callback) {
            msg.success ? callback(Ok()) : callback(Err(msg.error));
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
    });
}

}
