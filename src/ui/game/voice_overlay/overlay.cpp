#include "overlay.hpp"

#include "overlay_cell.hpp"
#include <audio/voice_playback_manager.hpp>
#include <managers/profile_cache.hpp>

using namespace geode::prelude;

bool GlobedVoiceOverlay::init() {
    if (!CCNode::init()) return false;

    this->setLayout(
        ColumnLayout::create()
            ->setGap(3.f)
            ->setAxisAlignment(AxisAlignment::Start)
            ->setCrossAxisLineAlignment(AxisAlignment::End)
    );

    this->setContentHeight(CCDirector::get()->getWinSize().height);

    return true;
}

void GlobedVoiceOverlay::addPlayer(int accountId) {
    auto& pcm = ProfileCacheManager::get();
    auto data = pcm.getData(accountId);

    PlayerAccountData accdata = PlayerAccountData::DEFAULT_DATA;
    if (data.has_value()) {
        accdata = data.value();
    }

    auto* cell = VoiceOverlayCell::create(accdata);
    this->addChild(cell);
}

void GlobedVoiceOverlay::updateOverlay() {
    auto& vpm = VoicePlaybackManager::get();

    this->removeAllChildren();

    vpm.forEachStream([this](int accountId, AudioStream& stream) {
        if (!stream.starving && (!PlayLayer::get()->m_level->isPlatformer() || stream.getVolume() > 0.005f)) {
            this->addPlayer(accountId);
        }
    });

    this->updateLayout();
}

void GlobedVoiceOverlay::updateOverlaySoft() {
    auto& vpm = VoicePlaybackManager::get();

    for (auto* cell : CCArrayExt<VoiceOverlayCell*>(this->getChildren())) {
        cell->updateVolume(vpm.getLoudness(cell->accountId));
    }
}

GlobedVoiceOverlay* GlobedVoiceOverlay::create() {
    auto ret = new GlobedVoiceOverlay;
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}