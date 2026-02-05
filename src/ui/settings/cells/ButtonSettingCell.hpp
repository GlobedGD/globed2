#pragma once

#include "BaseSettingCell.hpp"

#include <Geode/utils/function.hpp>

namespace globed {

class ButtonSettingCell : public BaseSettingCell<ButtonSettingCell> {
public:
    using Callback = geode::Function<void()>;

    static ButtonSettingCell* create(
        CStr name,
        CStr desc,
        CStr btnText,
        Callback&& cb,
        cocos2d::CCSize cellSize
    );

protected:
    Callback m_callback;
    CStr m_btnText;
    CCMenuItemSpriteExtra* m_button = nullptr;

    void setup() override;
    void reload() override {}
    void createButton(CStr text);
};

}