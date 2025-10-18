#pragma once

#include "singleton.hpp"
#include <asp/sync/SpinLock.hpp>
#include <std23/move_only_function.h>

namespace globed {

class FunctionQueue : public SingletonNodeBase<FunctionQueue, true> {
    friend class SingletonNodeBase;
    FunctionQueue() = default;

public:
    using Func = std23::move_only_function<void()>;
    asp::SpinLock<std::vector<Func>> m_queue;

    void queue(Func&& func);
    void update(float dt) override;
};

}
