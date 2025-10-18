#pragma once

#include "BaseSettingCell.hpp"

#include <std23/move_only_function.h>

namespace globed {

class ButtonSettingCell : public BaseSettingCell<ButtonSettingCell> {
public:
    using Callback = std23::move_only_function<void()>;

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