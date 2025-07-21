#pragma once
#include <atomic>

namespace globed {

class SpinLock {
    std::atomic_flag m_flag = ATOMIC_FLAG_INIT;

public:
    void rawLock() {
        while (m_flag.test_and_set(std::memory_order::acquire));
    }

    void rawUnlock() {
        m_flag.clear(std::memory_order::release);
    }

    struct Guard {
        SpinLock& m_lock;

        Guard(SpinLock& lock) : m_lock(lock) {
            m_lock.rawLock();
        }

        ~Guard() {
            m_lock.rawUnlock();
        }
    };

    Guard lock() {
        return Guard(*this);
    }
};

}
