#include "IntSliderSettingCell.hpp"

using namespace geode::prelude;

namespace globed {

void IntSliderSettingCell::setup() {
    FloatSettingCell::setup();

    m_slider->setCallback([this](auto, double value) {
        this->set<int>(value);
        this->reload();
    });
    m_slider->setStep(1.0);
}

void IntSliderSettingCell::reload() {
    int value = this->get<int>();
    m_slider->setValue(value);
    m_label->setString(fmt::to_string(value).c_str());

    m_slider->getParent()->updateLayout();
    m_rightMenu->updateLayout();
}

IntSliderSettingCell* IntSliderSettingCell::create(
    CStr key,
    CStr name,
    CStr desc,
    CCSize cellSize
) {
    auto ret = new IntSliderSettingCell;
    if (ret->init(key, name, desc, cellSize)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

}