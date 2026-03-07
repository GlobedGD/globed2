#pragma once

#include <globed/config.hpp>
#include <arc/future/PollableMetadata.hpp>

#include <Geode/binding/ObjectManager.hpp>
#include <Geode/binding/BitmapFontCache.hpp>
#include <string_view>
#include <cocos2d.h>
#include <fmt/format.h>

namespace globed {

[[noreturn]] void destructedSingleton(std::string_view name);
GLOBED_DLL void scheduleUpdateFor(cocos2d::CCObject* obj);

template <typename T>
constexpr std::string_view getTypenameConstexpr() {
    auto [p, size] = arc::getTypename<T>();
    return {p, size};
}

template <typename Derived>
class SingletonBase {
public:
    // no copy
    SingletonBase(const SingletonBase&) = delete;
    SingletonBase& operator=(const SingletonBase&) = delete;
    // no move
    SingletonBase(SingletonBase&&) = delete;
    SingletonBase& operator=(SingletonBase&&) = delete;

#ifdef GLOBED_BUILD
    GLOBED_DLL static Derived& get() {
        static Derived instance;

        if (destructed) {
            globed::destructedSingleton(globed::getTypenameConstexpr<Derived>());
        }

        return instance;
    }
#else
    GLOBED_DLL static Derived& get();
#endif

private:
    friend Derived;
    static inline bool destructed = false;

    SingletonBase() = default;
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

#ifdef GLOBED_BUILD
    GLOBED_DLL static Derived& get() {
        static Derived* instance = new Derived();
        return *instance;
    }
#else
    GLOBED_DLL static Derived& get();
#endif

private:
    friend Derived;
    SingletonLeakBase() = default;
};

// This is like SingletonLeakBase but it is also a CCObject with optional update schedule
template <typename Derived, bool ScheduleUpdate = false, typename Base = cocos2d::CCObject>
class SingletonNodeBase : public Base {
public:
    SingletonNodeBase(const SingletonNodeBase&) = delete;
    SingletonNodeBase& operator=(const SingletonNodeBase&) = delete;

    SingletonNodeBase(SingletonNodeBase&&) = delete;
    SingletonNodeBase& operator=(SingletonNodeBase&&) = delete;

#ifdef GLOBED_BUILD
    GLOBED_DLL static Derived& get() {
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
#else
    GLOBED_DLL static Derived& get();
#endif

private:
    friend Derived;
    SingletonNodeBase() = default;
};

// Singleton cache, for cocos/GD classes
template <typename T>
T* singleton() {
    // For some types, this is risky as their pointers may get invalidated by purging
    if constexpr (
        std::is_same_v<T, cocos2d::CCSpriteFrameCache>
        || std::is_same_v<T, cocos2d::CCTextureCache>
        || std::is_same_v<T, cocos2d::CCFileUtils>
        || std::is_same_v<T, BitmapFontCache>
        || std::is_same_v<T, ObjectManager>
    ) {
        return T::get();
    }

    static auto instance = T::get();
    return instance;
}


}