#include "slider_wrapper.hpp"

using namespace geode::prelude;

bool SliderWrapper::init(Slider* slider) {
    if (!CCNode::init()) return false;
    this->slider = slider;

    this->addChild(slider);
    slider->setContentSize(slider->m_groove->getScaledContentSize());
    this->setContentSize(slider->getScaledContentSize());

    slider->setPosition(slider->getScaledContentSize() / 2);

    return true;
}

SliderWrapper* SliderWrapper::create(Slider* slider) {
    auto ret = new SliderWrapper;
    if (ret->init(slider)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}