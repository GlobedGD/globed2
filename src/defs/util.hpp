#pragma once
#include "basic.hpp"

// singleton classes

template <typename Derived>
class SingletonBase {
public:
    SingletonBase(const SingletonBase&) = delete;
    SingletonBase& operator=(const SingletonBase&) = delete;

    static Derived& get() {
        static Derived instance;
        return instance;
    }

protected:
    SingletonBase() = default;
    virtual ~SingletonBase() = default;
};

#define GLOBED_SINGLETON(cls) public SingletonBase<cls>
