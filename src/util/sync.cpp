#include "sync.hpp"

#include <util/debug.hpp>

namespace util::sync {

template <>
void SmartThread<>::start() {
    _storage->_stopped.clear();
    _handle = std::thread([_storage = _storage]() {
        if (!_storage->threadName.empty()) {
            geode::utils::thread::setName(_storage->threadName);
        }

        try {
            while (!_storage->_stopped) {
                _storage->loopFunc();
            }
        } catch (const singleton_use_after_dtor& e) {
            log::warn("{}", e.what());
        } catch (const std::exception& e) {
            log::error("Terminating. Uncaught exception from thread {}: {}", _storage->threadName, e.what());
            util::debug::delayedSuicide(fmt::format("Uncaught exception from thread \"{}\": {}", _storage->threadName, e.what()));
        }

    });
}

ThreadPool::ThreadPool(size_t tc) {
    size_t id = rand() % 1024;

    for (size_t i = 0; i < tc; i++) {
        SmartThread<> thread;
        thread.setName(fmt::format("Thread {} (pool {})", i, id));
        thread.setLoopFunction([this, i = i] {
            auto& worker = this->workers.at(i);

            this->taskQueue.waitForMessages();

            worker.doingWork = true;

            auto task = this->taskQueue.tryPop();
            if (task) {
                task.value()();
            }

            worker.doingWork = false;
        });

        Worker worker = {
            .thread = std::move(thread),
            .doingWork = false
        };

        workers.emplace_back(std::move(worker));
    }

    for (auto& worker : workers) {
        worker.thread.start();
    }
}

ThreadPool::~ThreadPool() {
    try {
        this->join();
        this->workers.clear();
    } catch (const std::exception& e) {
        log::warn("failed to cleanup thread pool: {}", e.what());
    }
}

void ThreadPool::pushTask(const Task& task) {
    taskQueue.push(task);
}

void ThreadPool::pushTask(Task&& task) {
    taskQueue.push(std::move(task));
}

void ThreadPool::join() {
    while (!taskQueue.empty()) {
        std::this_thread::sleep_for(util::time::millis(2));
    }

    // wait for working threads to finish
    bool stillWorking;

    do {
        std::this_thread::sleep_for(util::time::millis(2));
        stillWorking = false;
        for (const auto& worker : workers) {
            if (worker.doingWork) {
                stillWorking = true;
                break;
            }
        }
    } while (stillWorking);
}

bool ThreadPool::isDoingWork() {
    if (!taskQueue.empty()) return true;

    for (const auto& worker : workers) {
        if (worker.doingWork) {
            return true;
        }
    }

    return false;
}

}