#pragma once

#include <config.hpp>

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
