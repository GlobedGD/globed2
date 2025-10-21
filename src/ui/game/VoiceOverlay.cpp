#include "VoiceOverlay.hpp"
#include <core/hooks/GJBaseGameLayer.hpp>
#include <globed/audio/AudioManager.hpp>
#include <globed/core/PlayerCacheManager.hpp>

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

void VoiceOverlay::update(float dt) {
    auto& am = AudioManager::get();
    am.forEachStream([&](int id, AudioStream& stream) {
        if (id <= 0) return;

        stream.updateEstimator(dt);
        this->updateStream(id, stream.isStarving(), stream.getVolume(), stream.getLoudness());
    });

    this->updateLayout();
}

void VoiceOverlay::updateSoft() {

}

void VoiceOverlay::updateStream(int id, bool starving, float volume, float loudness) {
    bool shouldShow = volume > 0.005f && !starving;
    auto it = m_cells.find(id);

    if (!shouldShow) {
        if (it != m_cells.end()) {
            it->second->removeFromParent();
            m_cells.erase(it);
        }

        return;
    }

    auto& pcm = PlayerCacheManager::get();

    // recreate if the cell either doesn't exist or if it's not initialized but data is available
    bool shouldRecreate = it == m_cells.end() || (it->second->getAccountId() == 0 && pcm.has(id));

    if (shouldRecreate) {
        if (it != m_cells.end()) {
            it->second->removeFromParent();
            m_cells.erase(it);
        }

        auto data = pcm.getOrDefault(id);

        auto cell = VoiceOverlayCell::create(data);
        this->addChild(cell);
        it = m_cells.emplace(id, cell).first;
    }

    it->second->updateLoudness(loudness);
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