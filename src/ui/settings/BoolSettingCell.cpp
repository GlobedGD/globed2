#include "BoolSettingCell.hpp"

#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace globed {

void BoolSettingCell::setup() {
    auto toggler = Build(CCMenuItemExt::createTogglerWithStandardSprites(0.75f, [this](auto toggler) {
        bool on = !toggler->isOn();
        this->set(on);
    }))
        .pos(m_size.width - 20.f, m_size.height / 2.f)
        .parent(this)
        .collect();

    toggler->toggle(this->get<bool>());
}

}