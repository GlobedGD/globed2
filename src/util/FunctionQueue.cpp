#include <globed/util/FunctionQueue.hpp>

using namespace geode::prelude;
using namespace asp::time;

namespace globed {

void FunctionQueue::queue(Func &&func, size_t frames)
{
    auto guard = m_queue.lock();
    guard->push(Queued{
        .expTick = m_tick + frames,
        .func = std::move(func),
    });
}

void FunctionQueue::queueDelay(Func &&func, asp::time::Duration delay)
{
    auto guard = m_delayedQueue.lock();
    guard->push(Delayed{
        .expiry = Instant::now() + delay,
        .func = std::move(func),
    });
}

void FunctionQueue::update(float dt)
{
    auto tick = m_tick++;

    auto guard = m_queue.lock();

    // Move all expired functions from m_queue to m_executing, this avoids locking the mutex for long and in best case
    // does not allocate
    while (!guard->empty()) {
        auto &top = guard->top();
        if (top.expTick > tick) {
            break;
        }

        m_executing.emplace_back(std::move(top.func));
        guard->pop();
    }

    guard.unlock();

    // Do the similar dance for delayed functions
    auto delayedGuard = m_delayedQueue.lock();
    auto now = Instant::now();

    while (!delayedGuard->empty()) {
        auto &top = delayedGuard->top();
        if (top.expiry > now) {
            break;
        }

        m_executing.emplace_back(std::move(top.func));
        delayedGuard->pop();
    }

    delayedGuard.unlock();

    for (auto &func : m_executing) {
        func();
    }

    m_executing.clear();
}

} // namespace globed