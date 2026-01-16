#pragma once

#include "BaseSettingCell.hpp"

namespace globed {

class IntCornerSettingCell : public BaseSettingCell<IntCornerSettingCell> {
protected:
    CCNode *m_innerSquare;

    void setup() override;
    void reload() override;
};

} // namespace globed