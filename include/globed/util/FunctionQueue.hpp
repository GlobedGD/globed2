#pragma once

#include "singleton.hpp"
#include <asp/sync/SpinLock.hpp>
#include <asp/time/Duration.hpp>
#include <asp/time/Instant.hpp>
#include <std23/move_only_function.h>

namespace globed {

class GLOBED_DLL FunctionQueue : public SingletonNodeBase<FunctionQueue, true> {
    friend class SingletonNodeBase;
    FunctionQueue() = default;

public:
    using Func = std23::move_only_function<void()>;
    struct Delayed {
        asp::time::Instant addedAt;
        asp::time::Duration delay;
        Func func;
    };

    asp::SpinLock<std::vector<Func>> m_queue;
    asp::SpinLock<std::vector<Delayed>> m_delayedQueue;

    void queue(Func&& func);
    void queueDelay(Func&& func, asp::time::Duration delay);
    void update(float dt) override;

private:
    std::vector<Func> m_executing;
};

}
