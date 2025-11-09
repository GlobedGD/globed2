#include <globed/util/FunctionQueue.hpp>

using namespace geode::prelude;
using namespace asp::time;

namespace globed {

void FunctionQueue::queue(Func&& func) {
    auto guard = m_queue.lock();
    guard->emplace_back(std::move(func));
}

void FunctionQueue::queueDelay(Func&& func, asp::time::Duration delay) {
    auto guard = m_delayedQueue.lock();
    guard->emplace_back(Delayed{
        .addedAt = asp::time::Instant::now(),
        .delay = delay,
        .func = std::move(func),
    });
}

void FunctionQueue::update(float dt) {
    auto guard = m_queue.lock();

    // Move all functions from m_queue to m_executing, this avoids locking the mutex for long and in best case does not allocate
    m_executing.reserve(m_executing.size() + guard->size());
    std::move(guard->begin(), guard->end(), std::back_inserter(m_executing));
    guard->clear();

    guard.unlock();

    // Do the similar dance for delayed functions
    auto delayedGuard = m_delayedQueue.lock();
    auto now = Instant::now();

    size_t i = 0;
    while (i < delayedGuard->size()) {
        auto& delayed = (*delayedGuard)[i];

        if (now.durationSince(delayed.addedAt) < delayed.delay) {
            ++i;
            continue;
        }

        m_executing.emplace_back(std::move(delayed.func));
        delayedGuard->erase(delayedGuard->begin() + i);
    }

    delayedGuard.unlock();

    for (auto& func : m_executing) {
        func();
    }

    m_executing.clear();
}

}