#include "FloatSettingCell.hpp"

#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace globed {

void FloatSettingCell::setup() {
    auto container = Build<CCNode>::create()
        .layout(SimpleColumnLayout::create()
            ->setMainAxisDirection(AxisDirection::BottomToTop)
            ->setMainAxisScaling(AxisScaling::Fit)
            ->setCrossAxisScaling(AxisScaling::Fit)
            ->setGap(0.f)
        )
        .parent(m_rightMenu)
        .collect();

    Build<AxisGap>::create(3.f)
        .parent(container);

    m_slider = Build(createSlider())
        .scale(0.9f)
        .contentSize(80.f, 18.f)
        .parent(container);

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
        .parent(container);

    this->reload();
}

void FloatSettingCell::reload() {
    float value = this->get<float>();
    m_slider->setValue(value);
    m_label->setString(fmt::format("{}%", (int)(value * 100)).c_str());

    m_slider->getParent()->updateLayout();
    m_rightMenu->updateLayout();
}

}