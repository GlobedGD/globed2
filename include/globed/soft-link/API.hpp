#pragma once
#include "Wrappers.hpp"

namespace globed::api::net {

// threadSafe = true means event will be invoked earlier and on the arc thread
template <typename T, typename F>
[[nodiscard("listen returns a listener that must be kept alive to receive messages")]]
MessageListener<T> listen(F&& callback, int priority = 0, bool threadSafe = false) {
    return MessageEvent<T>(threadSafe).listen(std::forward<F>(callback), priority);
}

template <typename T, typename F>
geode::ListenerHandle* listenGlobal(F&& callback, int priority = 0, bool threadSafe = false) {
    return MessageEvent<T>(threadSafe).listen(std::forward<F>(callback), priority).leak();
}

}
