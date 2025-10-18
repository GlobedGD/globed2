#include <globed/util/FunctionQueue.hpp>

namespace globed {

void FunctionQueue::queue(Func&& func) {
    auto guard = m_queue.lock();
    guard->emplace_back(std::move(func));
}

void FunctionQueue::update(float dt) {
    auto guard = m_queue.lock();
    auto funcs = std::move(*guard);
    guard->clear();
    guard.unlock();

    for (auto& func : funcs) {
        func();
    }
}

}