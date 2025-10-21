#include "VoiceOverlayCell.hpp"
#include <globed/util/gd.hpp>
#include <ui/misc/NameLabel.hpp>

#include <cue/Util.hpp>
#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace globed {

bool VoiceOverlayCell::init(const PlayerDisplayData& data) {
    CCNode::init();

    m_wrapper = Build<CCNode>::create()
        .parent(this)
        .layout(RowLayout::create()->setGap(5.f)->setAutoScale(false)->setAxisReverse(true))
        .id("container");

    m_accountId = data.accountId;

    m_visualizer = Build<AudioVisualizer>::create()
        .scale(0.5f)
        .scaleX(0.4f)
        .parent(m_wrapper)
        .id("visualizer");

    auto nameLabel = Build(NameLabel::create(data.username, "bigFont.fnt"))
        .with([&](auto lbl) {
            lbl->setScale(0.525f);
            if (data.specialUserData) {
                lbl->updateWithRoles(*data.specialUserData);
            }
        })
        .parent(m_wrapper)
        .collect();

    auto icon = Build<cue::PlayerIcon>::create(convertPlayerIcons(data.icons))
        .scale(0.45f)
        .parent(m_wrapper)
        .collect();

    const float heightMult = 1.3f;
    const float widthMult = 1.1f;

    m_wrapper->setContentWidth(icon->getScaledContentWidth() + 5.f + nameLabel->getScaledContentWidth() + 5.f + m_visualizer->getScaledContentWidth());
    m_wrapper->setContentHeight(icon->getScaledContentHeight() * heightMult);
    m_wrapper->updateLayout();

    // background
    auto bg = cue::attachBackground(m_wrapper, cue::BackgroundOptions {
        .sidePadding = 5.f,
        .verticalPadding = 3.f,
        .cornerRoundness = 1.5f
    });
    this->setContentSize(bg->getScaledContentSize());

    m_wrapper->setPosition({
        this->getScaledContentWidth() - m_wrapper->getScaledContentWidth() - 5.f,
        3.5f
    });

    return true;
}

void VoiceOverlayCell::updateLoudness(float loudness) {
    m_visualizer->setVolume(loudness);
}

VoiceOverlayCell* VoiceOverlayCell::create(const PlayerDisplayData& data) {
    auto ret = new VoiceOverlayCell;
    if (ret->init(data)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

}
