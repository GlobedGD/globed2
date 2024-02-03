#pragma once
#include "basic.hpp"
#include "platform.hpp"

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

// using decls to avoid polluting the namespace in headers

// ugly workaround because MSVC sucks ass
namespace __globed_log_namespace_shut_up_msvc {
    namespace log = geode::log;
}

using namespace __globed_log_namespace_shut_up_msvc;

using geode::Ref;
using geode::Loader;
using geode::Mod;
using geode::Result;
using geode::Ok;
using geode::Err;
using geode::cocos::CCArrayExt;
using geode::cast::typeinfo_cast;
