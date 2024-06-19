#include "name_label.hpp"

#include <util/ui.hpp>

using namespace geode::prelude;

bool GlobedNameLabel::init(const std::string& name, cocos2d::CCSprite* badgeSprite, const RichColor& nameColor) {
    if (!CCNode::init()) return false;

    this->setAnchorPoint({0.5f, 0.5f});
    this->setLayout(RowLayout::create()->setGap(4.f)->setAutoScale(false));
    this->setContentWidth(150.f);
    this->updateData(name, badgeSprite, nameColor);

    return true;
}

void GlobedNameLabel::updateData(const std::string& name, cocos2d::CCSprite* badgeSprite, const RichColor& nameColor) {
    this->updateName(name);
    this->updateBadge(badgeSprite);
    this->updateColor(nameColor);
}

void GlobedNameLabel::updateData(const std::string& name, const SpecialUserData& sud) {
    this->updateData(name, util::ui::createBadgeIfSpecial(sud), util::ui::getNameRichColor(sud));
}

void GlobedNameLabel::updateBadge(cocos2d::CCSprite* badgeSprite) {
    if (badge) badge->removeFromParent();

    badge = badgeSprite;

    if (badge) {
        util::ui::rescaleToMatch(badge, util::ui::BADGE_SIZE);
        badge->setZOrder(1);
        this->addChild(badge);
    }

    this->updateLayout();
}

void GlobedNameLabel::updateName(const std::string& name) {
    this->updateName(name.c_str());
}

void GlobedNameLabel::updateName(const char* name) {
    if (!label) {
        Build<CCNode>::create()
            .parent(this)
            .zOrder(-2)
            .store(labelContainer);

        Build<CCLabelBMFont>::create("", "chatFont.fnt")
            .zOrder(-1)
            .anchorPoint(0.f, 0.f)
            .parent(labelContainer)
            .store(label);

        Build<CCLabelBMFont>::create("", "chatFont.fnt")
            .zOrder(-2)
            .color(0, 0, 0)
            .anchorPoint(0.f, 0.0f)
            .pos(0.75f, -0.75f)
            .parent(labelContainer)
            .opacity(label->getOpacity() * 0.75f)
            .store(labelShadow);
    }

    label->setString(name);
    labelShadow->setString(name);
    labelContainer->setScaledContentSize(label->getScaledContentSize());
    this->updateLayout();
}

void GlobedNameLabel::updateOpacity(float opacity) {
    this->updateOpacity(static_cast<unsigned char>(opacity * 255.f));
}

void GlobedNameLabel::updateOpacity(unsigned char opacity) {
    if (label) label->setOpacity(opacity);
    if (badge) badge->setOpacity(opacity);
    if (labelShadow) labelShadow->setOpacity(opacity * 0.75);
}

void GlobedNameLabel::updateColor(const RichColor& color) {
    if (!label) return;

    color.animateLabel(label);
}

GlobedNameLabel* GlobedNameLabel::create(const std::string& name, cocos2d::CCSprite* badgeSprite, const RichColor& nameColor) {
    auto ret = new GlobedNameLabel;
    if (ret->init(name, badgeSprite, nameColor)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

GlobedNameLabel* GlobedNameLabel::create(const std::string& name, const SpecialUserData& sud) {
    return create(name, util::ui::createBadgeIfSpecial(sud), util::ui::getNameRichColor(sud));
}

GlobedNameLabel* GlobedNameLabel::create(const std::string& name) {
    return create(name, static_cast<CCSprite*>(nullptr), RichColor({255, 255, 255}));
}
