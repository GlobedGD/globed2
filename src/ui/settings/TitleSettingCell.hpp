#pragma once

#include "BaseSettingCell.hpp"

namespace globed {

class TitleSettingCell : public BaseSettingCell<TitleSettingCell> {
protected:
    void setup();
};

}