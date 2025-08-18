#include "FloatSettingCell.hpp"

#include <UIBuilder.hpp>
#include <ui/misc/Sliders.hpp>

using namespace geode::prelude;

namespace globed {

void FloatSettingCell::setup() {
    auto slider = Build(createSlider())
        .contentSize(60.f, 18.f)
        .pos(m_size.width - 40.f, m_size.height / 2.f)
        .parent(this)
        .collect();

    slider->setValue(this->get<float>());
}

}