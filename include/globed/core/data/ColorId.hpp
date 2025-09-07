#pragma once
#include <limits>
#include <stdint.h>
#include <Geode/binding/GameManager.hpp>

namespace globed {

template <typename T, T None = std::numeric_limits<T>::max()>
struct ColorId {
    ColorId(T val) : value(val) {}
    ColorId() : value(None) {}

    ColorId& operator=(T val) {
        this->value = val;
        return *this;
    }

    cocos2d::ccColor3B asColor() const {
        return GameManager::get()->colorForIdx(this->asIdx());
    }

    T inner() const {
        return value;
    }

    int asIdx() const {
        return this->isNone() ? (int)-1 : (int)value;
    }

    bool isNone() const {
        return value == None;
    }

private:
    T value;
};

}