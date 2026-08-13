#pragma once

#include "BaseSettingCell.hpp"

#include <Geode/utils/function.hpp>
#include <Geode/ui/Button.hpp>

namespace globed {

class ButtonSettingCell : public BaseSettingCell<ButtonSettingCell> {
public:
    using Callback = geode::Function<void()>;

    static ButtonSettingCell* create(
        ZStringView name,
        ZStringView desc,
        ZStringView btnText,
        Callback&& cb,
        cocos2d::CCSize cellSize
    );

protected:
    Callback m_callback;
    ZStringView m_btnText;
    geode::Button* m_button = nullptr;

    void setup() override;
    void reload() override {}
    void createButton(ZStringView text);
};

}