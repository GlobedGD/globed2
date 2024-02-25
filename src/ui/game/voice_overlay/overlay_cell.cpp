#include "overlay_cell.hpp"

using namespace geode::prelude;

bool VoiceOverlayCell::init(const PlayerAccountData& data) {
    if (!CCNode::init()) return false;

    accountId = data.accountId;

    this->setLayout(RowLayout::create()->setGap(5.f)->setAutoScale(false));

    auto gm = GameManager::get();
    auto color1 = gm->colorForIdx(data.icons.color1);
    auto color2 = gm->colorForIdx(data.icons.color2);

    auto playerIcon = Build<SimplePlayer>::create(data.icons.cube)
        .color(color1)
        .secondColor(color2)
        .scale(0.45f)
        .pos(0.f, 0.f)
        .parent(this)
        .collect();

    playerIcon->setContentSize(playerIcon->m_firstLayer->getScaledContentSize());
    auto firstNode = static_cast<CCNode*>(playerIcon->getChildren()->objectAtIndex(0));
    firstNode->setPosition(playerIcon->m_firstLayer->getScaledContentSize() / 2);

    if (data.icons.glowColor != -1) {
        playerIcon->setGlowOutline(gm->colorForIdx(data.icons.glowColor));
    }

    ccColor3B nameColor = ccColor3B(255, 255, 255);
    if (data.specialUserData.has_value()) {
        nameColor = data.specialUserData.value().nameColor;
    }

    auto nameLabel = Build<CCLabelBMFont>::create(data.name.c_str(), "bigFont.fnt")
        .scale(0.35f)
        .color(nameColor)
        .parent(this)
        .collect();

    Build<GlobedAudioVisualizer>::create()
        .scale(0.5f)
        .parent(this)
        .store(visualizer);

    visualizer->setScaleX(0.4f);

    this->setContentWidth(playerIcon->getScaledContentSize().width + 5.f + nameLabel->getScaledContentSize().width + 5.f + visualizer->getScaledContentSize().width);

    this->updateLayout();

    // background
    auto sizeScale = this->getScaledContentSize() * CCPoint{2.7f, 2.25f};
    auto cc9s = Build<CCScale9Sprite>::create("square02_001.png")
        .contentSize(this->getScaledContentSize() * sizeScale * 3.f)
        .scaleY(1.f / (sizeScale.height * 2.25f))
        .scaleX(1.f / (sizeScale.width * 2.7f))
        .opacity(80)
        .zOrder(-1)
        .anchorPoint(0.f, 0.f)
        .pos(-5.f, -2.5f)
        .parent(this)
        .collect();

    this->setContentSize(cc9s->getScaledContentSize());

    return true;
}

void VoiceOverlayCell::updateVolume(float vol) {
    visualizer->setVolume(vol);
}

VoiceOverlayCell* VoiceOverlayCell::create(const PlayerAccountData& data) {
    auto ret = new VoiceOverlayCell;
    if (ret->init(data)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}