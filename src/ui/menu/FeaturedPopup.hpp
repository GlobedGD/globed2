#pragma once

#include "FeaturedPopup.hpp"
#include "FeaturedLevelCell.hpp"

#include <globed/prelude.hpp>
#include <ui/BasePopup.hpp>

namespace globed {

class FeaturedPopup : public BasePopup {
public:
    static FeaturedPopup* create();

private:
    FeaturedLevelCell* m_cell;

    bool init();
};

}
