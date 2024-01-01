#pragma once
#include "basic.hpp"
#include "platform.hpp"
#include <stdexcept>

#define GLOBED_MBO(src, type, offset) *(type*)((char*)src + offset)

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
        return instance;
    }

protected:
    SingletonBase() = default;
    virtual ~SingletonBase() = default;
};

#define GLOBED_SINGLETON(cls) public SingletonBase<cls>
