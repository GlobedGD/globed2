#pragma once

#include <cocos2d.h>
#include <utility>

namespace globed {

// ccobject that stores a custom struct
template <typename T> class CCData : public cocos2d::CCObject {
    T inner;

    CCData(T data) : inner(std::move(data)) {}

    template <typename... Args> CCData(Args &&...args) : inner(std::forward<Args>(args)...) {}

public:
    template <typename... Args> static CCData *create(Args &&...args)
    {
        auto ret = new CCData{std::forward<Args>(args)...};
        ret->autorelease();
        return ret;
    }

    T &data()
    {
        return inner;
    }

    T *operator->() const
    {
        return &inner;
    }

    T &operator*() const
    {
        return inner;
    }
};

} // namespace globed
