#pragma once

#include <defs/platform.hpp>

namespace globed {
    [[noreturn]] void destructedSingleton();
}

// there was no reason to do this other than for me to learn crtp
template <typename Derived>
class SingletonBase {
public:
    // no copy
    SingletonBase(const SingletonBase&) = delete;
    SingletonBase& operator=(const SingletonBase&) = delete;
    // no move
    SingletonBase(SingletonBase&&) = delete;
    SingletonBase& operator=(SingletonBase&&) = delete;

    static Derived& get() {
        static Derived instance;

        if (destructed) {
            globed::destructedSingleton();
        }

        return instance;
    }

protected:
    static inline bool destructed = false;

    SingletonBase() {}
    virtual ~SingletonBase() {
        destructed = true;
    }
};

// This is like SingletonBase except not freed during program exit
template <typename Derived>
class SingletonLeakBase {
public:
    // no copy
    SingletonLeakBase(const SingletonLeakBase&) = delete;
    SingletonLeakBase& operator=(const SingletonLeakBase&) = delete;
    // no move
    SingletonLeakBase(SingletonLeakBase&&) = delete;
    SingletonLeakBase& operator=(SingletonLeakBase&&) = delete;

    static Derived& get() {
        static Derived* instance = new Derived();
        return *instance;
    }


protected:
    SingletonLeakBase() {}
};

namespace globed {
    // Singleton cache, for cocos/GD classes
    template <typename T>
    T* cachedSingleton() {
        static auto instance = T::get();
        return instance;
    }
}
