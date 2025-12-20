#pragma once

#include <Geode/binding/CCMenuItemSpriteExtra.hpp>
#include <cocos2d.h>
#include <globed/util/CStr.hpp>
#include <globed/core/SettingsManager.hpp>

namespace globed {

class BaseSettingCellBase : public cocos2d::CCMenu {
public:
    virtual void reload() {}

protected:
    CStr m_key;
    CStr m_name;
    CStr m_desc;
    cocos2d::CCSize m_size;
    CCMenuItemSpriteExtra* m_infoButton = nullptr;

    bool init(CStr key, CStr name, CStr desc, cocos2d::CCSize cellSize);
    bool initNoSetting(CStr name, CStr desc, cocos2d::CCSize cellSize);

    virtual void setup() = 0;
};

template <typename Derived>
class BaseSettingCell : public BaseSettingCellBase {
public:
    static Derived* create(
        CStr key,
        CStr name,
        CStr desc,
        cocos2d::CCSize cellSize
    ) {
        auto ret = new Derived;
        if (ret->init(key, name, desc, cellSize)) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }

protected:
    template <typename T>
    void set(T&& value) {
        globed::setting<std::decay_t<T>>(m_key) = std::forward<T>(value);
    }

    template <typename T>
    T get() {
        return globed::setting<T>(m_key);
    }
};

}