#pragma once

#include "BaseSettingCell.hpp"

namespace globed {

class FloatSettingCell : public BaseSettingCell<FloatSettingCell> {
protected:
    cocos2d::CCLabelBMFont* m_label;
    void setup();
};

}