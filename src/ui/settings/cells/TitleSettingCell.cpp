#include "TitleSettingCell.hpp"

#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace globed {

void TitleSettingCell::setup() {
    this->removeAllChildren();

    m_infoButton = nullptr;

    Build<Label>::create(m_name.c_str(), "goldFont.fnt")
        .scale(0.65f)
        .pos(m_size / 2.f + CCSize{0.f, 2.f})
        .parent(this);
}

}