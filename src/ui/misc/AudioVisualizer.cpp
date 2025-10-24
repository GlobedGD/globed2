#include "AudioVisualizer.hpp"

#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace globed {

bool AudioVisualizer::init() {
    CCNode::init();

    m_vis = Build<FMODLevelVisualizer>::create()
        .anchorPoint(0.5f, 0.5f)
        .rotation(90.f)
        .parent(this);

    auto batch = m_vis->getChildByType<CCSpriteBatchNode>(0);
    auto bigBar = batch->getChildByType<CCSprite>(0);

    CCSize visSize = bigBar->getScaledContentSize();

    // invert these
    this->setContentSize({visSize.height, visSize.width});
    m_vis->setPosition({visSize.height / 2.f, visSize.width / 2.f});

    return true;
}

void AudioVisualizer::setVolume(float vol) {
    this->setVolumeRaw(vol * 1.f);
}

void AudioVisualizer::setVolumeRaw(float vol) {
    m_maxVolume = std::max(m_maxVolume, vol);
    m_vis->updateVisualizer(vol, m_maxVolume, 0.f);
}

void AudioVisualizer::resetMaxVolume() {
    m_maxVolume = 0.f;
}

void AudioVisualizer::setScaleX(float scale) {
    CCNode::setScaleX(scale);

    // do not try this at home
    auto batchnode = this->m_vis->getChildByType<CCSpriteBatchNode>(0);
    auto border = static_cast<CCSprite*>(batchnode->getChildren()->objectAtIndex(0));
    border->setScaleY(100.f + 1.f / scale);
}

AudioVisualizer* AudioVisualizer::create() {
    auto ret = new AudioVisualizer;
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

}
