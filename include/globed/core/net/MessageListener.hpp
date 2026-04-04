#pragma once

#include "../../config.hpp"
#include <Geode/loader/Event.hpp>
#include <Geode/utils/function.hpp>

namespace globed {

template <typename T>
struct MessageEvent : geode::ThreadSafeGlobalEvent<MessageEvent<T>, bool(T&), bool> {
    using geode::ThreadSafeGlobalEvent<MessageEvent<T>, bool(T&), bool>::ThreadSafeGlobalEvent;
};

template <typename T>
struct MessageListener {
    MessageListener() = default;
    MessageListener(geode::ListenerHandle handle) : m_handle(std::move(handle)) {}
    MessageListener(const MessageListener&) = delete;
    MessageListener& operator=(const MessageListener&) = delete;
    MessageListener(MessageListener&&) = default;
    MessageListener& operator=(MessageListener&&) = default;

    void reset() {
        m_handle = {};
    }

private:
    geode::ListenerHandle m_handle;
};

}
