#pragma once

#include "BaseSettingCell.hpp"

namespace globed {

class FloatSettingCell : public BaseSettingCell<FloatSettingCell> {
protected:
    void setup();
};

}