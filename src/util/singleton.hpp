#pragma once

#include <defs/platform.hpp>
#include <cocos2d.h>

namespace globed {
    [[noreturn]] void destructedSingleton(std::string_view name);
    void scheduleUpdateFor(cocos2d::CCObject* obj);

    // had to be copied from util::debug because it includes this file
    template <typename T>
    constexpr std::string_view _sgetTypenameConstexpr() {
#ifdef __clang__
        constexpr auto pfx = sizeof("std::string_view getTypenameConstexpr() [T = ") - 1;
        constexpr auto sfx = sizeof("]") - 1;
        constexpr auto function = __PRETTY_FUNCTION__;
        constexpr auto len = sizeof(__PRETTY_FUNCTION__) - pfx - sfx - 1;
        return {function + pfx, len};
#else
        static_assert(false, "well well well");
#endif
    }
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
            globed::destructedSingleton(globed::_sgetTypenameConstexpr<Derived>());
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

// This is like SingletonLeakBase but it is also a CCObject with optional update schedule
template <typename Derived, bool ScheduleUpdate = false, typename Base = cocos2d::CCObject>
class SingletonNodeBase : public Base {
public:
    SingletonNodeBase(const SingletonNodeBase&) = delete;
    SingletonNodeBase& operator=(const SingletonNodeBase&) = delete;

    SingletonNodeBase(SingletonNodeBase&&) = delete;
    SingletonNodeBase& operator=(SingletonNodeBase&&) = delete;

    static Derived& get() {
        static Derived* obj = []{
            auto obj = new Derived();

            if constexpr (requires { obj->init(); }) {
                obj->init();
            }

            if constexpr (requires { obj->onEnter(); }) {
                obj->onEnter();
            }

            if constexpr (ScheduleUpdate) {
                globed::scheduleUpdateFor(obj);
            }

            return obj;
        }();

        return *obj;
    }

protected:
    SingletonNodeBase() {}
};

namespace globed {
    // Singleton cache, for cocos/GD classes
    template <typename T>
    T* cachedSingleton() {
        static auto instance = T::get();
        return instance;
    }
}
