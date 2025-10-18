#pragma once

#include <globed/config.hpp>
#include <globed/core/data/PlayerState.hpp>
#include <globed/core/data/PlayerDisplayData.hpp>

#include <std23/move_only_function.h>
#include <cstring>

namespace globed {

enum class ListenerResult {
    /// Continue executing the listener chain.
    Continue,
    /// Stop executing the listener chain, if a listener returns this, it will be the last listener to be called with this message.
    Stop,
};

template <typename T>
using ListenerFn = std23::move_only_function<ListenerResult (const T&)>;

void _destroyListener(const std::type_info& ty, void* ptr);

struct MessageListenerImplBase {
    int m_priority = 0;
    bool m_threadSafe = false;
    alignas(8) uint8_t m_reserved[31];
};

template <typename T>
class MessageListenerImpl : MessageListenerImplBase {
public:
    MessageListenerImpl(const MessageListenerImpl&) = delete;
    MessageListenerImpl& operator=(const MessageListenerImpl&) = delete;
    MessageListenerImpl(MessageListenerImpl&&) noexcept = default;
    MessageListenerImpl& operator=(MessageListenerImpl&&) noexcept = default;

    /// Set the priority of this listener. Lower values are called first. The default is 0.
    void setPriority(int priority) {
        m_priority = priority;
    }

    int getPriority() const {
        return m_priority;
    }

    /// Set whether the listener is thread-safe. If `false` (the default), the listener will always be called on the main thread.
    void setThreadSafe(bool threadSafe) {
        m_threadSafe = threadSafe;
    }

    bool isThreadSafe() const {
        return m_threadSafe;
    }

private:
    friend class NetworkManagerImpl;

    ListenerFn<T> m_callback;

    inline MessageListenerImpl(ListenerFn<T> callback) : m_callback(std::move(callback)) {
        std::memset(m_reserved, 0, sizeof(m_reserved));
    }

    ListenerResult invoke(const T& message) {
        if (m_callback) {
            return m_callback(message);
        }

        return ListenerResult::Continue;
    }
};

template <typename T>
class MessageListener {
public:
    MessageListener(const MessageListener&) = delete;
    MessageListener& operator=(const MessageListener&) = delete;

    MessageListener(MessageListener&& other) noexcept
        : m_impl(other.m_impl) {
        other.m_impl = nullptr;
    }

    MessageListener& operator=(MessageListener&& other) noexcept {
        if (this != &other) {
            if (m_impl) {
                _destroyListener(typeid(T), m_impl);
            }
            m_impl = other.m_impl;
            other.m_impl = nullptr;
        }
        return *this;
    }

    MessageListenerImpl<T>* operator*() {
        return m_impl;
    }

    MessageListenerImpl<T>* operator->() {
        return m_impl;
    }

    ~MessageListener() {
        if (m_impl) {
            _destroyListener(typeid(T), m_impl);
            m_impl = nullptr;
        }
    }

    /// Set the priority of this listener. Lower values are called first. The default is 0.
    void setPriority(int priority) {
        m_impl->setPriority(priority);
    }

    int getPriority() const {
        return m_impl->getPriority();
    }

    /// Set whether the listener is thread-safe. If `false` (the default), the listener will always be called on the main thread.
    void setThreadSafe(bool threadSafe) {
        m_impl->setThreadSafe(threadSafe);
    }

    bool isThreadSafe() const {
        return m_impl->isThreadSafe();
    }

private:
    friend class NetworkManagerImpl;
    MessageListenerImpl<T>* m_impl;

    MessageListener(MessageListenerImpl<T>* impl) : m_impl(impl) {}
};

}