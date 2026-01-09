#include "FloatSettingCell.hpp"

#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace globed {

void FloatSettingCell::setup() {
    m_slider = Build(createSlider())
        .scale(0.9f)
        .contentSize(80.f, 18.f)
        .pos(m_size.width - 48.f, m_size.height / 2.f - 1.f)
        .parent(this)
        .collect();

    // get limits
    auto limits = SettingsManager::get().getLimits(m_key);
    if (limits && limits->first.isNumber() && limits->second.isNumber()) {
        m_slider->setRange(limits->first.asDouble().unwrap(), limits->second.asDouble().unwrap());
    }

    m_slider->setCallback([this](auto, double value) {
        this->set(value);
        this->reload();
    });

    m_label = Build<CCLabelBMFont>::create("", "bigFont.fnt")
        .scale(0.3f)
        .pos(m_size.width - 48.f, m_size.height / 2.f + 9.f)
        .parent(this)
        .collect();

    this->reload();
}

void FloatSettingCell::reload() {
    float value = this->get<float>();
    m_slider->setValue(value);
    m_label->setString(fmt::format("{}%", (int)(value * 100)).c_str());
}

}