#pragma once

#include "BaseSettingCell.hpp"

namespace globed {

class ButtonSettingCell : public BaseSettingCell<ButtonSettingCell> {
public:
    using Callback = std::function<void()>;

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

    void setup();
};

}