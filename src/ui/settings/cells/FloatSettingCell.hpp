#pragma once

#include "BaseSettingCell.hpp"
#include <ui/Core.hpp>

namespace globed {

class FloatSettingCell : public BaseSettingCell<FloatSettingCell> {
public:
    void setPercentageBased(bool percentage);
    void setStep(float step);

protected:
    cocos2d::CCLabelBMFont* m_label;
    cue::Slider* m_slider;
    bool m_isPercentage = true;

    void setup() override;
    void reload() override;
};

}