/*
* sync.hpp is a util file for syncing things across threads.
*/

#pragma once
#include <condition_variable>
#include <atomic>
#include <memory>
#include <queue>
#include <mutex>
#include <shared_mutex>

#include <defs.hpp>
#include <util/data.hpp>

/*
* SmartMessageQueue is a utility wrapper around std::queue,
* that allows you to synchronously push/pop messages from multiple threads,
* and additionally even block the thread until new messages are available.
*/

namespace util::sync {

template <typename T>
class SmartMessageQueue {
public:
    SmartMessageQueue() {}
    void waitForMessages() {
        std::unique_lock lock(_mtx);
        if (!_iq.empty()) return;

        _cvar.wait(lock, [this](){ return !_iq.empty(); });
    }

    // returns true if messages are available, otherwise false if returned because of timeout.
    bool waitForMessages(std::chrono::seconds timeout) {
        std::unique_lock lock(_mtx);
        if (!_iq.empty()) return true;
        
        return _cvar.wait_for(lock, timeout, [this](){ return !_iq.empty(); });
    }

    bool empty() const {
        std::lock_guard lock(_mtx);
        return _iq.empty();
    }

    size_t size() const {
        std::lock_guard lock(_mtx);
        return _iq.size();
    }

    T pop() {
        std::lock_guard lock(_mtx);
        auto val = _iq.front();
        _iq.pop();
        return val;
    }

    std::vector<T> popAll() {
        std::vector<T> out;
        std::lock_guard lock(_mtx);
        
        while (!_iq.empty()) {
            out.push_back(_iq.front());
            _iq.pop();
        }

        return out;
    }

    void push(T msg, bool notify = true) {
        std::lock_guard lock(_mtx);
        _iq.push(msg);
        if (notify)
            _cvar.notify_all();
    }

    template <typename Iter>
    void pushAll(const Iter& iterable, bool notify = true) {
        std::lock_guard lock(_mtx);
        std::copy(std::begin(iterable), std::end(iterable), std::back_inserter(_iq));

        if (notify)
            _cvar.notify_all();
    }
private:
    std::queue<T> _iq;
    mutable std::mutex _mtx;
    std::condition_variable _cvar;
};

/*
* WrappingMutex is a mutex lock that holds an object and
* allows you to access it via a RAII lock guard
*/

template <typename T>
class WrappingMutex {
public:
    WrappingMutex(): data_(std::make_shared<T>()), mutex_() {}
    WrappingMutex(T obj) : data_(std::make_shared(obj)), mutex_() {}

    class Guard {
    public:
        Guard(std::shared_ptr<T> data, std::mutex& mutex) : mutex_(mutex), data_(data) {
            mutex_.lock();
        }
        ~Guard() {
            if (!alreadyUnlocked)
                mutex_.unlock();
        }
        T& operator* () {
            return *data_;
        }
        T* operator->() {
            return data_.get();
        }
        Guard& operator=(const T& rhs) {
            *data_ = rhs;
            return *this;
        }
        // Calling unlock and trying to use the guard afterwards is undefined behavior!
        void unlock() {
            if (!alreadyUnlocked) {
                mutex_.unlock();
                alreadyUnlocked = true;
            }
        }

    private:
        std::shared_ptr<T> data_;
        std::mutex& mutex_;
        bool alreadyUnlocked = false;
    };

    Guard lock() {
        return Guard(data_, mutex_);
    }

private:
    std::shared_ptr<T> data_;
    std::mutex mutex_;
};

/*
* WrappingRwLock is a readers-writer lock that holds an objecet
* and allows you to access it via a RAII lock guard.
*
* Multiple threads can read the data at once, but when a thread wants to write,
* no one else is allowed access.
*/

template <typename T>
class WrappingRwLock {
public:
    WrappingRwLock() : data_(std::make_shared<T>()), rw_mutex_() {}
    WrappingRwLock(T obj) : data_(std::make_shared<T>(obj)), rw_mutex_() {}

    class ReadGuard {
    public:
        ReadGuard(std::shared_ptr<T> data, std::shared_mutex& rw_mutex) : rw_mutex_(rw_mutex), data_(data) {
            rw_mutex_.lock_shared();
        }

        ~ReadGuard() {
            rw_mutex_.unlock_shared();
        }

        const T& operator*() const {
            return *data_;
        }

        const T* operator->() const {
            return data_.get();
        }

    private:
        std::shared_ptr<T> data_;
        std::shared_mutex& rw_mutex_;
    };

    class WriteGuard {
    public:
        WriteGuard(std::shared_ptr<T> data, std::shared_mutex& rw_mutex) : rw_mutex_(rw_mutex), data_(data) {
            rw_mutex_.lock();
        }

        ~WriteGuard() {
            rw_mutex_.unlock();
        }

        T& operator*() {
            return *data_;
        }

        T* operator->() {
            return data_.get();
        }

    private:
        std::shared_ptr<T> data_;
        std::shared_mutex& rw_mutex_;
    };

    ReadGuard read() {
        return ReadGuard(data_, rw_mutex_);
    }

    WriteGuard write() {
        return WriteGuard(data_, rw_mutex_);
    }

private:
    std::shared_ptr<T> data_;
    mutable std::shared_mutex rw_mutex_;
};

// simple wrapper around atomics with the default memory order set to relaxed
template <typename T>
class RelaxedAtomic {
public:
    RelaxedAtomic(T initial = {}) : value(initial) {}

    T load(std::memory_order order = std::memory_order::relaxed) const {
        return value.load(order);
    }

    void store(T val, std::memory_order order = std::memory_order::relaxed) {
        value.store(val, order);
    }

    operator T() const {
        return load();
    }

    RelaxedAtomic<T>& operator=(T val) {
        store(val);
        return *this;
    }
private:
    std::atomic<T> value;
};

using AtomicBool = RelaxedAtomic<bool>;
using AtomicChar = RelaxedAtomic<char>;
using AtomicByte = RelaxedAtomic<data::byte>;
using AtomicI16 = RelaxedAtomic<int16_t>;
using AtomicU16 = RelaxedAtomic<uint16_t>;
using AtomicInt = RelaxedAtomic<int>;
using AtomicI32 = RelaxedAtomic<int32_t>;
using AtomicU32 = RelaxedAtomic<uint32_t>;
using AtomicI64 = RelaxedAtomic<int64_t>;
using AtomicU64 = RelaxedAtomic<uint64_t>;

}