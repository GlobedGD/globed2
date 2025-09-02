#include "NameLabel.hpp"

#include <UIBuilder.hpp>
#include <cue/Util.hpp>

using namespace geode::prelude;

namespace globed {

bool NameLabel::init(const std::string& name, const char* font, bool alignMiddle) {
    if (!CCMenu::init()) return false;

    m_font = font;
    m_shadow = false;

    auto bcLayout = RowLayout::create()->setAutoScale(false)->setGap(3.f);
    bcLayout->ignoreInvisibleChildren(true);
    m_badgeContainer = Build<CCNode>::create()
        .zOrder(99)
        .layout(bcLayout)
        .parent(this);

    this->setAnchorPoint({0.5f, 0.5f});
    this->ignoreAnchorPointForPosition(false);
    this->setLayout(RowLayout::create()
                ->setGap(4.f)
                ->setAutoScale(false)
                ->setAxisAlignment(alignMiddle ? AxisAlignment::Center : AxisAlignment::Start)
    );
    this->setContentWidth(300.f);
    this->updateName(name);

    return true;
}

void NameLabel::updateName(const std::string& name) {
    this->updateName(name.c_str());
}

void NameLabel::updateName(const char* name) {
    if (!m_label) {
        m_labelContainer = Build<CCNode>::create()
            .zOrder(-2);

        Build<CCLabelBMFont>::create("", m_font)
            .zOrder(-1)
            .anchorPoint(0.f, 0.f)
            .parent(m_labelContainer)
            .store(m_label);

        Build<CCLabelBMFont>::create("", m_font)
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
    m_labelShadow->setVisible(m_shadow);
    m_labelContainer->setScaledContentSize(m_label->getScaledContentSize());

    if (m_labelButton) {
        m_labelButton->removeFromParent();
    }

    m_labelButton = Build(*m_labelContainer)
        .intoMenuItem([this](auto btn) {
            this->onClick(btn);
        })
        .scaleMult(1.1f)
        .enabled(false)
        .parent(this);

    this->updateLayout();
}

void NameLabel::onClick(CCMenuItemSpriteExtra* btn) {
    if (m_callback) {
        m_callback(btn);
    }
}

void NameLabel::updateTeam(size_t idx, cocos2d::ccColor4B color) {
    // TODO: make this an option

    bool colorbmode = false;

    if (!colorbmode) {
        if (m_label) {
            m_label->setColor(cue::into<ccColor3B>(color));
            m_label->setOpacity(color.a);
        }
    } else {
        if (!m_teamLabel) {
            m_teamLabel = Build<CCLabelBMFont>::create("", "bigFont.fnt")
                .parent(m_badgeContainer);
        }

        m_teamLabel->setString(fmt::format("{}", idx + 1).c_str());
        m_teamLabel->setColor(cue::into<ccColor3B>(color));

        this->resizeBadgeContainer();
        this->updateLayout();
    }
}

void NameLabel::updateNoTeam() {
    m_teamLabel->setVisible(false);
    this->resizeBadgeContainer();
    this->updateLayout();
}

void NameLabel::resizeBadgeContainer() {
    size_t elems = 0;
    float width = 0.f;

    for (auto elem : m_badgeContainer->getChildrenExt()) {
        if (!elem->isVisible()) continue;
        elems++;
        width += elem->getScaledContentWidth();
    }

    width += (elems - 1) * 3.f;

    m_badgeContainer->setContentWidth(width);
    m_badgeContainer->updateLayout();
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

void NameLabel::makeClickable(std::function<void(CCMenuItemSpriteExtra*)> callback) {
    m_callback = callback;
    m_labelButton->setEnabled(true);
}

void NameLabel::setMultipleBadges(bool multiple) {
    m_multipleBadges = multiple;

    // TODO
}

void NameLabel::setShadowEnabled(bool enabled) {
    m_shadow = enabled;

    if (m_labelShadow) {
        m_labelShadow->setVisible(enabled);
    }
}

NameLabel* NameLabel::create(const std::string& name, const char* font, bool alignMiddle) {
    auto ret = new NameLabel();
    if (ret->init(name, font, alignMiddle)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

}