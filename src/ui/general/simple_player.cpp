#include "simple_player.hpp"

using namespace geode::prelude;

bool GlobedSimplePlayer::init(const Icons& icons) {
    if (!CCNode::init()) return false;

    Build<SimplePlayer>::create(0)
        .parent(this)
        .store(sp);

    this->setContentSize(sp->m_outlineSprite->getScaledContentSize());
    sp->setPosition(sp->m_outlineSprite->getScaledContentSize() / 2);
    this->updateIcons(icons);

    return true;
}

void GlobedSimplePlayer::updateIcons(const Icons& icons) {
    this->icons = icons;
    this->updateIcons();
}

void GlobedSimplePlayer::updateIcons() {
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

SimplePlayer* GlobedSimplePlayer::getInner() {
    return sp;
}

GlobedSimplePlayer* GlobedSimplePlayer::create(const Icons& icons) {
    auto ret = new GlobedSimplePlayer();
    if (ret->init(icons)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

GlobedSimplePlayer* GlobedSimplePlayer::create(IconType type, const PlayerIconData& icons) {
    return create(Icons(
        type,
        util::misc::getIconWithType(icons, type),
        icons.color1,
        icons.color2,
        icons.glowColor == NO_GLOW ? -1 : (int)icons.glowColor
    ));
}

GlobedSimplePlayer* GlobedSimplePlayer::create(const PlayerIconData& icons) {
    return create(IconType::Cube, icons);
}
