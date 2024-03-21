#pragma once
#include <stdexcept>

#include <config.hpp>
#include "platform.hpp"

#define GLOBED_MBO(src, type, offset) *(type*)((char*)src + offset)

class singleton_use_after_dtor : public std::runtime_error {
public:
    singleton_use_after_dtor() : std::runtime_error("attempting to use a singleton after static destruction") {}
};

// singleton classes

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
            throw singleton_use_after_dtor();
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
