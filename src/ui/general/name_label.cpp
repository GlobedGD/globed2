#include "name_label.hpp"

#include <util/ui.hpp>

using namespace geode::prelude;

bool GlobedNameLabel::init(const std::string& name, cocos2d::CCSprite* badgeSprite, cocos2d::ccColor3B nameColor) {
    if (!CCNode::init()) return false;

    this->setAnchorPoint({0.5f, 0.5f});
    this->setLayout(RowLayout::create()->setGap(4.f));
    this->setContentWidth(150.f);
    this->updateData(name, badgeSprite, nameColor);

    return true;
}

void GlobedNameLabel::updateData(const std::string& name, cocos2d::CCSprite* badgeSprite, ccColor3B nameColor) {
    this->updateName(name);
    this->updateBadge(badgeSprite);
    this->updateColor(nameColor);
}

void GlobedNameLabel::updateData(const std::string& name, std::optional<SpecialUserData> sud) {
    if (sud) {
        this->updateData(name, util::ui::createBadgeIfSpecial(sud->nameColor), sud->nameColor);
    } else {
        this->updateData(name, nullptr, {255, 255, 255});
    }
}

void GlobedNameLabel::updateBadge(cocos2d::CCSprite* badgeSprite) {
    if (badge) badge->removeFromParent();

    badge = badgeSprite;

    if (badge) {
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
        Build<CCLabelBMFont>::create("", "chatFont.fnt")
            .zOrder(-1)
            .parent(this)
            .store(label);
    }

    label->setString(name);
    this->updateLayout();
}

void GlobedNameLabel::updateOpacity(float opacity) {
    this->updateOpacity(static_cast<unsigned char>(opacity * 255.f));
}

void GlobedNameLabel::updateOpacity(unsigned char opacity) {
    if (label) label->setOpacity(opacity);
    if (badge) badge->setOpacity(opacity);
}

void GlobedNameLabel::updateColor(cocos2d::ccColor3B color) {
    if (label) label->setColor(color);
}

GlobedNameLabel* GlobedNameLabel::create(const std::string& name, cocos2d::CCSprite* badgeSprite, cocos2d::ccColor3B nameColor) {
    auto ret = new GlobedNameLabel;
    if (ret->init(name, badgeSprite, nameColor)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

GlobedNameLabel* GlobedNameLabel::create(const std::string& name, std::optional<SpecialUserData> sud) {
    if (sud) {
        return create(name, util::ui::createBadgeIfSpecial(sud->nameColor), sud->nameColor);
    } else {
        return create(name, nullptr, {255, 255, 255});
    }
}

GlobedNameLabel* GlobedNameLabel::create(const std::string& name) {
    return create(name, static_cast<CCSprite*>(nullptr), {255, 255, 255});
}
