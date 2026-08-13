#pragma once

#include <Geode/ui/Button.hpp>
#include <globed/prelude.hpp>
#include <globed/core/SettingsManager.hpp>

namespace globed {

class BaseSettingCellBase : public cocos2d::CCMenu {
public:
    virtual void reload() {}

protected:
    ZStringView m_key;
    ZStringView m_name;
    ZStringView m_desc;
    CCSize m_size;
    CCMenu* m_rightMenu;
    Button* m_infoButton = nullptr;

    bool init(ZStringView key, ZStringView name, ZStringView desc, CCSize cellSize);
    bool initNoSetting(ZStringView name, ZStringView desc, CCSize cellSize);

    virtual void setup() = 0;
};

template <typename Derived>
class BaseSettingCell : public BaseSettingCellBase {
public:
    static Derived* create(
        ZStringView key,
        ZStringView name,
        ZStringView desc,
        CCSize cellSize
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