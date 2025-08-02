#include "NameLabel.hpp"

#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace globed {

bool NameLabel::init(const std::string& name, bool bigFont) {
    if (!CCNode::init()) return false;

    m_bigFont = bigFont;
    this->setAnchorPoint({0.5f, 0.5f});
    this->setLayout(RowLayout::create()->setGap(4.f)->setAutoScale(false));
    this->setContentWidth(300.f);
    this->updateData(name);

    return true;
}

void NameLabel::updateData(const std::string& name) {
    this->updateName(name);
}

void NameLabel::updateName(const std::string& name) {
    this->updateName(name.c_str());
}

void NameLabel::updateName(const char* name) {
    if (!m_label) {
        Build<CCNode>::create()
            .parent(this)
            .zOrder(-2)
            .store(m_labelContainer);

        Build<CCLabelBMFont>::create("", m_bigFont ? "bigFont.fnt" : "chatFont.fnt")
            .zOrder(-1)
            .anchorPoint(0.f, 0.f)
            .parent(m_labelContainer)
            .store(m_label);

        Build<CCLabelBMFont>::create("", m_bigFont ? "bigFont.fnt" : "chatFont.fnt")
            .zOrder(-2)
            .color(0, 0, 0)
            .anchorPoint(0.f, 0.0f)
            .pos(0.75f, -0.75f)
            .parent(m_labelContainer)
            .opacity(m_label->getOpacity() * 0.75f)
            .store(m_labelShadow);
    }

    m_label->setString(name);
    m_labelShadow->setString(name);
    m_labelContainer->setScaledContentSize(m_label->getScaledContentSize());
    this->updateLayout();
}

void NameLabel::updateOpacity(float opacity) {
    this->updateOpacity(static_cast<unsigned char>(opacity * 255.f));
}

void NameLabel::updateOpacity(unsigned char opacity) {
    if (m_label) {
        m_label->setOpacity(opacity);
        m_labelShadow->setOpacity(opacity * 0.75f);
    }

    if (m_badgeContainer) {
        for (auto child : m_badgeContainer->getChildrenExt<CCSprite>()) {
            child->setOpacity(opacity);
        }
    }
}

void NameLabel::setMultipleBadges(bool multiple) {
    m_multipleBadges = multiple;

    // TODO
}

NameLabel* NameLabel::create(const std::string& name, bool bigFont) {
    auto ret = new NameLabel();
    if (ret->init(name, bigFont)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

}