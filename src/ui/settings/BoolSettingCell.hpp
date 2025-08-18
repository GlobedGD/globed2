#pragma once

#include "BaseSettingCell.hpp"

namespace globed {

class BoolSettingCell : public BaseSettingCell<BoolSettingCell> {
protected:
    void setup();
};

}