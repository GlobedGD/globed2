#pragma once

#include "FloatSettingCell.hpp"

namespace globed {

class IntSliderSettingCell : public FloatSettingCell {
public:
    static IntSliderSettingCell *create(CStr key, CStr name, CStr desc, cocos2d::CCSize cellSize);

protected:
    void setup() override;
    void reload() override;
};

} // namespace globed