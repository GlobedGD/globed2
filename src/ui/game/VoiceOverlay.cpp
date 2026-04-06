#include "VoiceOverlay.hpp"
#include <core/hooks/GJBaseGameLayer.hpp>
#include <globed/audio/AudioManager.hpp>
#include <globed/core/PlayerCacheManager.hpp>

using namespace geode::prelude;
using namespace asp::time;

static constexpr float VOICE_OVERLAY_PAD_X = 5.f;
static float g_threshold = 0.f;

namespace globed {

bool VoiceOverlay::init() {
    CCNode::init();

    this->schedule(schedule_selector(VoiceOverlay::update), 1.f / 30.f);

    return true;
}

void VoiceOverlay::reposition() {
    auto winSize = CCDirector::get()->getWinSize();
    // im lazy so copied from ping overlay.cpp
    auto epos = globed::setting<int>("core.level.voice-overlay-position");

    bool onTop = epos < 2;
    bool onRight = epos % 2 == 1;

    float padY = globed::setting<float>("core.level.voice-overlay-pad-y");

    float oby = onTop ? winSize.height - padY : padY;
    float obx = onRight ? winSize.width - VOICE_OVERLAY_PAD_X : VOICE_OVERLAY_PAD_X;

    float anchorX = onRight ? 1.f : 0.f;
    float anchorY = onTop ? 1.f : 0.f;

    this->setLayout(ColumnLayout::create()
        ->setGap(3.f)
        ->setAxisReverse(!onTop)
        ->setAxisAlignment(onTop ? AxisAlignment::End : AxisAlignment::Start)
        ->setCrossAxisLineAlignment(onRight ? AxisAlignment::End : AxisAlignment::Start)
    );

    this->setContentHeight(winSize.height);

    this->setPosition(obx, oby);
    this->setAnchorPoint({anchorX, anchorY});
    this->updateLayout();
}

void VoiceOverlay::update(float dt) {
    auto gjbgl = GlobedGJBGL::get();
    for (auto& [id, player] : gjbgl->m_fields->m_players) {
        this->updateStream(*player, false);
    }
    this->updateStream(*gjbgl->m_fields->m_ghost, true);

    this->updateLayout();
}

void VoiceOverlay::updateSoft() {
}

void VoiceOverlay::updateStream(RemotePlayer& player, bool local) {
    auto stream = player.getVoiceStream();
    if (!stream) return;

    this->updateStream(
        local ? -1 : player.id(),
        stream->isStarving(),
        stream->getAudibility()
    );
}

void VoiceOverlay::updateStream(int id, bool starving, float loudness) {
    bool shouldShow = loudness >= g_threshold && !starving;
    auto it = m_cells.find(id);

    if (!shouldShow) {
        if (it != m_cells.end()) {
            // remove the cell if the user hasnt spoken in over a second
            auto cell = it->second;
            if (cell->sinceLastSpoken() > Duration::fromSecs(1)) {
                cell->removeFromParent();
                m_cells.erase(it);
            } else {
                cell->updateLoudness(loudness);
            }
        }

        return;
    }

    auto& pcm = PlayerCacheManager::get();

    // recreate if the cell either doesn't exist or if it's not initialized but data is available
    bool shouldRecreate = it == m_cells.end() || (id != -1 && it->second->getAccountId() == 0 && pcm.has(id));

    if (shouldRecreate) {
        if (it != m_cells.end()) {
            it->second->removeFromParent();
            m_cells.erase(it);
        }

        PlayerDisplayData data;

        if (id == -1) {
            data = PlayerDisplayData::getOwn();
        } else {
            data = pcm.getOrDefault(id);
        }

        auto cell = VoiceOverlayCell::create(data);
        this->addChild(cell);
        it = m_cells.emplace(id, cell).first;
    }

    it->second->updateLoudness(loudness);
    it->second->updateLastSpoken();
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

$on_mod(Loaded) {
    g_threshold = globed::setting<float>("core.level.voice-overlay-threshold");
    globed::SettingsManager::get().listenForChanges<double>("core.level.voice-overlay-threshold", [](double value) {
        g_threshold = value;
    });
}
