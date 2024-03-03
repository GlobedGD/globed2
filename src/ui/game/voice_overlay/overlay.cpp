#include "overlay.hpp"

#include "overlay_cell.hpp"
#include <audio/voice_playback_manager.hpp>
#include <hooks/play_layer.hpp>
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
#ifdef GLOBED_VOICE_SUPPORT
    auto& vpm = VoicePlaybackManager::get();

    this->removeAllChildren();

    bool isProximity = static_cast<GlobedPlayLayer*>(PlayLayer::get())->m_fields->isVoiceProximity;

    vpm.forEachStream([this, isProximity = isProximity](int accountId, AudioStream& stream) {
        if (!stream.starving && (!isProximity || stream.getVolume() > 0.005f)) {
            this->addPlayer(accountId);
        }
    });

    this->updateLayout();
#endif // GLOBED_VOICE_SUPPORT
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