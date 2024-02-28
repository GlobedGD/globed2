#include "sync.hpp"

#include <util/debug.hpp>

namespace util::sync {

template <>
void SmartThread<>::start() {
    _storage->_stopped.clear();
    _handle = std::thread([_storage = std::move(_storage)]() {
        if (!_storage->threadName.empty()) {
            geode::utils::thread::setName(_storage->threadName);
        }

        try {
            while (!_storage->_stopped) {
                _storage->loopFunc();
            }
        } catch (const std::exception& e) {
            log::error("Terminating. Uncaught exception from thread {}: {}", _storage->threadName, e.what());
            util::debug::delayedSuicide(fmt::format("Uncaught exception from thread \"{}\": {}", _storage->threadName, e.what()));
        }

    });
}

}