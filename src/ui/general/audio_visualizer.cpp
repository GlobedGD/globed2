#include "audio_visualizer.hpp"

using namespace geode::prelude;

bool GlobedAudioVisualizer::init() {
    if (!CCNode::init()) return false;

    Build<FMODLevelVisualizer>::create()
        .anchorPoint(0.5f, 0.5f)
        .rotation(90.0f)
        .id("fmod-visualizer"_spr)
        .parent(this)
        .store(visNode);

    auto batchnode = getChildOfType<CCSpriteBatchNode>(visNode, 0);
    auto bigBar = static_cast<CCSprite*>(batchnode->getChildren()->objectAtIndex(0));
    CCSize visualizerSize = bigBar->getScaledContentSize();

    this->setContentSize({visualizerSize.height, visualizerSize.width});
    visNode->setPosition({visualizerSize.height / 2, visualizerSize.width / 2});

    return true;
}

void GlobedAudioVisualizer::setVolume(float val) {
    this->setVolumeRaw(val * 2.f);
}

void GlobedAudioVisualizer::setVolumeRaw(float val) {
    if (val >= maxVolume) {
        maxVolume = val;
    }

    this->visNode->updateVisualizer(val, maxVolume, 0.f);
}

void GlobedAudioVisualizer::resetMaxVolume() {
    maxVolume = 0.f;
}

void GlobedAudioVisualizer::setScaleX(float scale) {
    CCNode::setScaleX(scale);

    // do not try this at home
    auto batchnode = getChildOfType<CCSpriteBatchNode>(this->visNode, 0);
    auto border = static_cast<CCSprite*>(batchnode->getChildren()->objectAtIndex(0));
    border->setScaleY(100.f + 1.f / scale);
}

GlobedAudioVisualizer* GlobedAudioVisualizer::create() {
    auto ret = new GlobedAudioVisualizer;
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}