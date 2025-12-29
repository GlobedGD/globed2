#pragma once

#include <globed/config.hpp>
#include <globed/core/data/PlayerState.hpp>
#include <globed/core/data/PlayerDisplayData.hpp>

#include <cstring>
#ifdef GLOBED_API_EXT_FUNCTIONS
# include <std23/move_only_function.h>
#endif

namespace globed {

enum class ListenerResult {
    /// Continue executing the listener chain.
    Continue,
    /// Stop executing the listener chain, if a listener returns this, it will be the last listener to be called with this message.
    Stop,
};

template <typename T>
using ListenerFn = std23::move_only_function<ListenerResult (const T&)>;

struct MessageListenerImplBase {
    int m_priority = 0;
    bool m_threadSafe = false;
    alignas(8) uint8_t m_reserved[31];

    void destroy(const std::type_info& ty);
};

template <typename T>
class MessageListenerImpl : public MessageListenerImplBase {
public:
    MessageListenerImpl(const MessageListenerImpl&) = delete;
    MessageListenerImpl& operator=(const MessageListenerImpl&) = delete;
    MessageListenerImpl(MessageListenerImpl&&) noexcept = default;
    MessageListenerImpl& operator=(MessageListenerImpl&&) noexcept = default;

    /// Set the priority of this listener. Lower values are called first. The default is 0.
    inline void setPriority(int priority) {
        m_priority = priority;
    }

    inline int getPriority() const {
        return m_priority;
    }

    /// Set whether the listener is thread-safe. If `false` (the default), the listener will always be called on the main thread.
    inline void setThreadSafe(bool threadSafe) {
        m_threadSafe = threadSafe;
    }

    inline bool isThreadSafe() const {
        return m_threadSafe;
    }

private:
    friend class NetworkManagerImpl;
    friend class NetworkManager;

    ListenerFn<T> m_callback;

    inline MessageListenerImpl(ListenerFn<T> callback) : m_callback(std::move(callback)) {
        std::memset(m_reserved, 0, sizeof(m_reserved));
    }

    inline ListenerResult invoke(const T& message) {
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

    inline MessageListener(MessageListener&& other) noexcept
        : m_impl(other.m_impl) {
        other.m_impl = nullptr;
    }

    inline MessageListener& operator=(MessageListener&& other) noexcept {
        if (this != &other) {
            if (m_impl) m_impl->destroy(typeid(T));
            m_impl = other.m_impl;
            other.m_impl = nullptr;
        }
        return *this;
    }

    inline MessageListenerImpl<T>* operator*() {
        return m_impl;
    }

    inline MessageListenerImpl<T>* operator->() {
        return m_impl;
    }

    inline ~MessageListener() {
        if (m_impl) m_impl->destroy(typeid(T));
    }

    /// Set the priority of this listener. Lower values are called first. The default is 0.
    inline void setPriority(int priority) {
        m_impl->setPriority(priority);
    }

    inline int getPriority() const {
        return m_impl->getPriority();
    }

    /// Set whether the listener is thread-safe. If `false` (the default), the listener will always be called on the main thread.
    inline void setThreadSafe(bool threadSafe) {
        m_impl->setThreadSafe(threadSafe);
    }

    inline bool isThreadSafe() const {
        return m_impl->isThreadSafe();
    }

private:
    friend class NetworkManagerImpl;
    friend class NetworkManager;
    MessageListenerImpl<T>* m_impl = nullptr;

    inline MessageListener(MessageListenerImpl<T>* impl) : m_impl(impl) {}
};

}