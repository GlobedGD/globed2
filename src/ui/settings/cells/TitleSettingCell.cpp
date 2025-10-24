#include "TitleSettingCell.hpp"

#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace globed {

void TitleSettingCell::setup() {
    this->removeAllChildren();

    m_infoButton = nullptr;

    Build<CCLabelBMFont>::create(m_name, "goldFont.fnt")
        .scale(0.7f)
        .pos(m_size / 2.f)
        .parent(this);
}

}