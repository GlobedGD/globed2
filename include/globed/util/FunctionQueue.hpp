#pragma once

#include "singleton.hpp"

#include <asp/sync/SpinLock.hpp>
#include <asp/time/Duration.hpp>
#include <asp/time/Instant.hpp>
#include <queue>
#include <std23/move_only_function.h>

namespace globed {

class GLOBED_DLL FunctionQueue : public SingletonNodeBase<FunctionQueue, true> {
    friend class SingletonNodeBase;
    FunctionQueue() = default;

public:
    using Func = std23::move_only_function<void()>;
    struct Queued {
        size_t expTick;
        mutable Func func; // xd priority queue funnies

        bool operator>(const Queued &other) const
        {
            return expTick > other.expTick;
        }
    };

    struct Delayed {
        asp::time::Instant expiry;
        mutable Func func;

        bool operator>(const Delayed &other) const
        {
            return expiry > other.expiry;
        }
    };

    asp::SpinLock<std::priority_queue<Queued, std::vector<Queued>, std::greater<Queued>>> m_queue;
    asp::SpinLock<std::priority_queue<Delayed, std::vector<Delayed>, std::greater<Delayed>>> m_delayedQueue;

    void queue(Func &&func, size_t frames = 0);
    void queueDelay(Func &&func, asp::time::Duration delay);
    void update(float dt) override;

private:
    std::vector<Func> m_executing;
    size_t m_tick = 0;
};

} // namespace globed
