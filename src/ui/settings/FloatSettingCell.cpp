#include "FloatSettingCell.hpp"

#include <UIBuilder.hpp>
#include <ui/misc/Sliders.hpp>

using namespace geode::prelude;

namespace globed {

void FloatSettingCell::setup() {
    auto slider = Build(createSlider())
        .contentSize(80.f, 18.f)
        .pos(m_size.width - 48.f, m_size.height / 2.f)
        .parent(this)
        .collect();

    slider->setCallback([this](auto, double value) {
        this->set(value);
        m_label->setString(fmt::format("{}%", (int)(value * 100)).c_str());
    });

    float val = this->get<float>();
    slider->setValue(val);

    m_label = Build<CCLabelBMFont>::create(fmt::format("{}%", (int)(val * 100)).c_str(), "bigFont.fnt")
        .scale(0.3f)
        .pos(m_size.width - 40.f, m_size.height / 2.f + 10.f)
        .parent(this)
        .collect();
}

}