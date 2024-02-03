#include "sync.hpp"

namespace util::sync {

template <>
void SmartThread<>::start() {
    _storage->_stopped.clear();
    _handle = std::thread([_storage = _storage]() {
        if (!_storage->threadName.empty()) {
            geode::utils::thread::setName(_storage->threadName);
        }

        while (!_storage->_stopped) {
            _storage->loopFunc();
        }
    });
}

}