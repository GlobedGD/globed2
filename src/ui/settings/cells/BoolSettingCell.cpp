#include "BoolSettingCell.hpp"

#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace globed {

void BoolSettingCell::setup() {
    m_toggler = Build(CCMenuItemExt::createTogglerWithStandardSprites(0.7f, [this](auto toggler) {
        bool on = !toggler->isOn();
        this->set(on);
    }))
        .posY(m_size.height / 2.f)
        .parent(this)
        .collect();

    m_toggler->setPositionX(m_size.width - 8.f - m_toggler->getContentWidth() / 2.f);

    m_toggler->m_offButton->m_scaleMultiplier = 1.15f;
    m_toggler->m_onButton->m_scaleMultiplier = 1.15f;

    this->reload();
}

void BoolSettingCell::reload() {
    m_toggler->toggle(this->get<bool>());
}

}