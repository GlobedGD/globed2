#include "simple_player.hpp"

using namespace geode::prelude;

bool GlobedSimplePlayer::init(const Icons& icons) {
    if (!CCNode::init()) return false;

    Build<SimplePlayer>::create(0)
        .parent(this)
        .store(sp);

    this->setContentSize(sp->m_firstLayer->getScaledContentSize());
    sp->setPosition(sp->m_firstLayer->getScaledContentSize() / 2);
    this->updateIcons(icons);

    return true;
}

void GlobedSimplePlayer::updateIcons(const Icons& icons) {
    this->icons = icons;

    auto* gm = GameManager::get();

    sp->updatePlayerFrame(icons.id, icons.type);

    sp->setColor(gm->colorForIdx(icons.color1));
    sp->setSecondColor(gm->colorForIdx(icons.color2));

    if (icons.color3 == -1) {
        sp->disableGlowOutline();
    } else {
        sp->setGlowOutline(gm->colorForIdx(icons.color3));
    }
}

GlobedSimplePlayer* GlobedSimplePlayer::create(const Icons& icons) {
    auto ret = new GlobedSimplePlayer;
    if (ret->init(icons)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}