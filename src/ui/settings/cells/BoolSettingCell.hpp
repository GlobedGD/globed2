#pragma once

#include "BaseSettingCell.hpp"

namespace globed {

class BoolSettingCell : public BaseSettingCell<BoolSettingCell> {
protected:
    CCMenuItemToggler *m_toggler;

    void setup() override;
    void reload() override;
};

} // namespace globed