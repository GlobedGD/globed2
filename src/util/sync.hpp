/*
* sync.hpp is a util file for syncing things across threads.
*/

#pragma once
#include <defs/essential.hpp>

#include <condition_variable>
#include <thread>
#include <atomic>
#include <queue>
#include <utility>

#include <util/data.hpp>
#include <util/time.hpp>

// forward decl because we can't include the header
namespace util::debug {
    void delayedSuicide(const std::string_view);
}

namespace util::sync {

/*
* SmartMessageQueue is a utility wrapper around std::queue,
* that allows you to synchronously push/pop messages from multiple threads,
* and additionally even block the thread until new messages are available.
*/
template <typename T>
class SmartMessageQueue {
public:
    SmartMessageQueue() {}
    void waitForMessages() {
        std::unique_lock lock(_mtx);
        if (!_iq.empty()) return;

        _cvar.wait(lock, [this] { return !_iq.empty(); });
    }

    // returns true if messages are available, otherwise false if returned because of timeout.
    template <typename Rep, typename Period>
    bool waitForMessages(util::time::duration<Rep, Period> timeout) {
        std::unique_lock lock(_mtx);
        if (!_iq.empty()) return true;

        return _cvar.wait_for(lock, timeout, [this] { return !_iq.empty(); });
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

    void push(const T& msg, bool notify = true) {
        std::lock_guard lock(_mtx);
        _iq.push(msg);
        if (notify)
            _cvar.notify_one();
    }

    template <typename Iter>
    void pushAll(const Iter& iterable, bool notify = true) {
        std::lock_guard lock(_mtx);
        std::copy(std::begin(iterable), std::end(iterable), std::back_inserter(_iq));

        if (notify)
            _cvar.notify_one();
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
    WrappingMutex(T&& obj) : data_(std::make_shared<T>(std::move(obj))), mutex_() {}

    class Guard {
    public:
        // no copy
        Guard(const Guard&) = delete;
        Guard& operator=(const Guard&) = delete;

        Guard(std::shared_ptr<T> data, std::mutex& mutex) : data_(data), mutex_(mutex) {
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

template <>
class WrappingMutex<void> {
public:
    WrappingMutex() : mutex_() {}

    class Guard {
    public:
        // no copy
        Guard(const Guard&) = delete;
        Guard& operator=(const Guard&) = delete;

        Guard(std::mutex& mutex) : mutex_(mutex) {
            mutex_.lock();
        }

        ~Guard() {
            if (!alreadyUnlocked)
                mutex_.unlock();
        }

        // Calling unlock and trying to use the guard afterwards is undefined behavior!
        void unlock() {
            if (!alreadyUnlocked) {
                mutex_.unlock();
                alreadyUnlocked = true;
            }
        }

    private:
        std::mutex& mutex_;
        bool alreadyUnlocked = false;
    };

    Guard lock() {
        return Guard(mutex_);
    }
private:
    std::mutex mutex_;
};

// simple wrapper around atomics with the default memory order set to relaxed instead of seqcst + copy constructor
template <typename T, typename Inner = std::atomic<T>>
class RelaxedAtomic {
public:
    // idk what im doing send help
    template <typename U = T, std::enable_if_t<!std::is_void_v<U>, U> = 0>
    RelaxedAtomic(T initial = {}) : value(initial) {}

    template <typename U = T, std::enable_if_t<!std::is_void_v<U>, U> = 0>
    T load(std::memory_order order = std::memory_order::relaxed) const {
        return value.load(order);
    }

    template <typename U = T, std::enable_if_t<!std::is_void_v<U>, U> = 0>
    void store(T val, std::memory_order order = std::memory_order::relaxed) {
        value.store(val, order);
    }

    template <typename U = T, std::enable_if_t<!std::is_void_v<U>, U> = 0>
    operator T() const {
        return load();
    }

    template <typename U = T, std::enable_if_t<!std::is_void_v<U>, U> = 0>
    RelaxedAtomic<T, Inner>& operator=(T val) {
        this->store(val);
        return *this;
    }

    // enable copying, it is disabled by default in std::atomic
    template <typename U = T, std::enable_if_t<!std::is_void_v<U>, U> = 0>
    RelaxedAtomic(const RelaxedAtomic<T, Inner>& other) {
        this->store(other.load());
    }

    template <typename U = T, std::enable_if_t<!std::is_void_v<U>, U> = 0>
    RelaxedAtomic<T, Inner>& operator=(const RelaxedAtomic<T, Inner>& other) {
        if (this != &other) {
            this->store(other.load());
        }

        return *this;
    }

    // enable moving
    template <typename U = T, std::enable_if_t<!std::is_void_v<U>, U> = 0>
    RelaxedAtomic(RelaxedAtomic<T, Inner>&& other) {
        this->store(other.load());
    }

    template <typename U = T, std::enable_if_t<!std::is_void_v<U>, U> = 0>
    RelaxedAtomic& operator=(RelaxedAtomic<T, Inner>&& other) {
        if (this != &other) {
            this->store(other.load());
        }

        return *this;
    }

    // enable move

private:
    Inner value;
};

template <>
class RelaxedAtomic<void, std::atomic_flag> {
public:
    RelaxedAtomic() {}

    bool test(std::memory_order order = std::memory_order::relaxed) const {
        return value.test(order);
    }

    void clear(std::memory_order order = std::memory_order::relaxed) {
        value.clear(order);
    }

    void set(std::memory_order order = std::memory_order::relaxed) {
        this->testAndSet(order);
    }

    bool testAndSet(std::memory_order order = std::memory_order::relaxed) {
        return value.test_and_set(order);
    }

    operator bool() const {
        return this->test();
    }

    RelaxedAtomic<void, std::atomic_flag>& operator=(bool val) {
        val ? this->set() : this->clear();
        return *this;
    }

    // copying
    RelaxedAtomic(const RelaxedAtomic<void, std::atomic_flag>& other) {
        other.test() ? this->set() : this->clear();
    }

    RelaxedAtomic<void, std::atomic_flag>& operator=(const RelaxedAtomic<void, std::atomic_flag>& other) {
        if (this != &other) {
            other.test() ? this->set() : this->clear();
        }

        return *this;
    }

    // moving
    RelaxedAtomic(RelaxedAtomic<void, std::atomic_flag>&& other) {
        other.test() ? this->set() : this->clear();
    }

    RelaxedAtomic<void, std::atomic_flag>& operator=(RelaxedAtomic<void, std::atomic_flag>&& other) {
        if (this != &other) {
            other.test() ? this->set() : this->clear();
        }

        return *this;
    }

private:
    std::atomic_flag value;
};

template <>
class RelaxedAtomic<float, std::atomic<uint32_t>> {
public:
    RelaxedAtomic(float initial = {}) : value(data::bit_cast<uint32_t>(initial)) {}

    float load(std::memory_order order = std::memory_order::relaxed) const {
        return data::bit_cast<float>(value.load(order));
    }

    void store(float val, std::memory_order order = std::memory_order::relaxed) {
        value.store(data::bit_cast<uint32_t>(val), order);
    }

    operator float() const {
        return this->load();
    }

    RelaxedAtomic<float, std::atomic<uint32_t>>& operator=(float val) {
        this->store(val);
        return *this;
    }

    // enable copying, it is disabled by default in std::atomic
    RelaxedAtomic(RelaxedAtomic<float, std::atomic<uint32_t>>& other) {
        this->store(other.load());
    }

    RelaxedAtomic<float, std::atomic<uint32_t>>& operator=(const RelaxedAtomic<float, std::atomic<uint32_t>>& other) {
        if (this != &other) {
            this->store(other.load());
        }

        return *this;
    }

    // moving
    RelaxedAtomic(RelaxedAtomic<float, std::atomic<uint32_t>>&& other) {
        this->store(other.load());
    }

    RelaxedAtomic<float, std::atomic<uint32_t>>& operator=(RelaxedAtomic<float, std::atomic<uint32_t>>&& other) {
        if (this != &other) {
            this->store(other.load());
        }

        return *this;
    }
private:
    std::atomic<uint32_t> value;
};

using AtomicFlag = RelaxedAtomic<void, std::atomic_flag>;
using AtomicBool = RelaxedAtomic<bool>;
using AtomicChar = RelaxedAtomic<char>;
using AtomicByte = RelaxedAtomic<data::byte>;
using AtomicI8 = RelaxedAtomic<int8_t>;
using AtomicU8 = RelaxedAtomic<uint8_t>;
using AtomicI16 = RelaxedAtomic<int16_t>;
using AtomicU16 = RelaxedAtomic<uint16_t>;
using AtomicInt = RelaxedAtomic<int>;
using AtomicI32 = RelaxedAtomic<int32_t>;
using AtomicU32 = RelaxedAtomic<uint32_t>;
using AtomicI64 = RelaxedAtomic<int64_t>;
using AtomicU64 = RelaxedAtomic<uint64_t>;
using AtomicF32 = RelaxedAtomic<float, std::atomic<uint32_t>>;
using AtomicSizeT = RelaxedAtomic<size_t>;

template<typename... TFuncArgs>
class SmartThread {
    using TFunc = std::function<void (TFuncArgs...)>;
public:
    SmartThread() {
        _storage = std::make_shared<Storage>();
    }

    SmartThread(const SmartThread&) = delete;
    SmartThread& operator=(const SmartThread&) = delete;

    SmartThread(SmartThread&& other) noexcept = default;
    SmartThread& operator=(SmartThread&& other) noexcept = default;

    SmartThread(const std::string_view name) {
        this->setName(name);
    }

    SmartThread(TFunc&& func) {
        this->setLoopFunction(std::move(func));
    }

    SmartThread(const std::string_view name, TFunc&& func) {
        this->setName(name);
        this->setLoopFunction(std::move(func));
    }

    void setLoopFunction(TFunc&& func) {
        _storage->loopFunc = std::move(func);
    }

    // Change the name of the thread. Must be called before the thread is started.
    void setName(const std::string_view name) {
        _storage->threadName = name;
    }

    void start(TFuncArgs&&... args) {
        _storage->_stopped.clear();
        _handle = std::thread([_storage = _storage](TFuncArgs&&... args) {
            if (!_storage->threadName.empty()) {
                geode::utils::thread::setName(_storage->threadName);
            }

            try {
                while (!_storage->_stopped) {
                    _storage->loopFunc(args...);
                }
            } catch (const std::exception& e) {
                util::debug::delayedSuicide(fmt::format("Uncaught exception from thread \"{}\": {}", _storage->threadName, e.what()));
            }
        }, std::forward<TFuncArgs>(args)...);
    }

    // Request the thread to be stopped as soon as possible
    void stop() {
        if (_storage) {
            _storage->_stopped.set();
        } else {
            throw std::runtime_error("tried to stop a detached SmartThread");
        }
    }

    // Join the thread if possible, else do nothing
    void join() {
        if (_handle.joinable()) _handle.join();
    }

    // Stop the thread and wait for it to terminate
    void stopAndWait() {
        this->stop();
        this->join();
    }

    // Detach the thread and let it execute even after this `SmartThread` object is destructed. This `SmartThread` instance must not be used afterwards.
    // NOTE: this intentionally leaks resources. The thread will be impossible to stop unless an exception occurs.
    void detach() {
        if (_handle.joinable()) _handle.detach();
        _handle = {};
        // release the ownership
        _storage.reset();
    }

    ~SmartThread() {
        this->stopAndWait();
    }

private:
    struct Storage {
        AtomicFlag _stopped;
        TFunc loopFunc;
        std::string threadName;
    };

    std::thread _handle;
    std::shared_ptr<Storage> _storage = nullptr;
};

template <>
void SmartThread<>::start();

}