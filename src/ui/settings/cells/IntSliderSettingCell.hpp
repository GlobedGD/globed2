#pragma once

#include "FloatSettingCell.hpp"

namespace globed {

class IntSliderSettingCell : public FloatSettingCell {
public:
    static IntSliderSettingCell* create(ZStringView key, ZStringView name, ZStringView desc, CCSize cellSize);

protected:
    void setup() override;
    void reload() override;
};

}