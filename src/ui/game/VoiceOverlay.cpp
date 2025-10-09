#include "VoiceOverlay.hpp"
#include <core/hooks/GJBaseGameLayer.hpp>

using namespace geode::prelude;

namespace globed {

bool VoiceOverlay::init() {
    CCNode::init();

    this->setLayout(
        ColumnLayout::create()
            ->setGap(3.f)
            ->setAxisAlignment(AxisAlignment::Start)
            ->setCrossAxisLineAlignment(AxisAlignment::End)
    );

    this->setContentHeight(CCDirector::get()->getWinSize().height);
    this->schedule(schedule_selector(VoiceOverlay::update), 1.f / 30.f);

    return true;
}

void VoiceOverlay::update(float) {
    // TODO (medium): implement
}

VoiceOverlay* VoiceOverlay::create() {
    auto ret = new VoiceOverlay;
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

}