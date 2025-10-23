#pragma once

#include "BaseSettingCell.hpp"
#include <ui/misc/Sliders.hpp>

namespace globed {

class FloatSettingCell : public BaseSettingCell<FloatSettingCell> {
protected:
    cocos2d::CCLabelBMFont* m_label;
    cue::Slider* m_slider;

    void setup() override;
    void reload() override;
};

}