#include "BoolSettingCell.hpp"

#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace globed {

void BoolSettingCell::setup()
{
    m_toggler = Build(CCMenuItemExt::createTogglerWithStandardSprites(0.7f,
                                                                      [this](auto toggler) {
                                                                          bool on = !toggler->isOn();
                                                                          this->set(on);
                                                                      }))
                    .posY(m_size.height / 2.f)
                    .parent(m_rightMenu)
                    .collect();
    m_rightMenu->updateLayout();

    m_toggler->m_offButton->m_scaleMultiplier = 1.15f;
    m_toggler->m_onButton->m_scaleMultiplier = 1.15f;

    this->reload();
}

void BoolSettingCell::reload()
{
    m_toggler->toggle(this->get<bool>());
}

} // namespace globed