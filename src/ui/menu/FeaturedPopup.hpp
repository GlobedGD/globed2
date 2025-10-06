#pragma once

#include "FeaturedPopup.hpp"
#include "FeaturedLevelCell.hpp"

#include <globed/prelude.hpp>
#include <ui/BasePopup.hpp>

namespace globed {

class FeaturedPopup : public BasePopup<FeaturedPopup> {
public:
    static const CCSize POPUP_SIZE;

private:
    FeaturedLevelCell* m_cell;

    bool setup();
};

}
