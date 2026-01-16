#include "NameLabel.hpp"
#include <core/net/NetworkManagerImpl.hpp>
#include <globed/core/SettingsManager.hpp>
#include <ui/misc/Badges.hpp>

#include <UIBuilder.hpp>
#include <cue/Util.hpp>

using namespace geode::prelude;

namespace globed {

// controls scale of the names
static constexpr float NAME_HEIGHT = 17.f;
static constexpr float MAX_NAME_WIDTH = 140.f;

bool NameLabel::init(const std::string &name, const char *font)
{
    if (!CCMenu::init())
        return false;

    m_font = font;
    m_shadow = false;

    auto bcLayout = SimpleRowLayout::create()
                        ->setMainAxisScaling(AxisScaling::None)
                        ->setCrossAxisScaling(AxisScaling::Fit)
                        ->setGap(3.f);

    bcLayout->ignoreInvisibleChildren(true);
    m_badgeContainer =
        Build<CCNode>::create().id("badge-container").anchorPoint(0.f, 0.5f).zOrder(99).layout(bcLayout).parent(this);

    this->setAnchorPoint({0.5f, 0.5f});
    this->ignoreAnchorPointForPosition(false);

    auto myLayout = SimpleRowLayout::create()
                        ->setGap(4.f)
                        ->setCrossAxisScaling(AxisScaling::Fit)
                        ->setMainAxisScaling(AxisScaling::Fit);
    myLayout->ignoreInvisibleChildren(true);

    this->setLayout(myLayout);
    this->updateName(name);

    if (std::string_view{font} == "chatFont.fnt") {
        this->setShadowEnabled(true);
    }

    return true;
}

void NameLabel::updateName(const std::string &name)
{
    this->updateName(name.c_str());
}

void NameLabel::updateName(const char *name)
{
    if (!m_label) {
        m_labelContainer = Build<CCNode>::create().zOrder(-2);

        Build<GradientLabel>::create("", m_font)
            .zOrder(-1)
            .anchorPoint(0.f, 0.f)
            .parent(m_labelContainer)
            .store(m_label);

        Build<Label>::create("", m_font)
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

    float yScale = NAME_HEIGHT / m_label->getContentHeight();
    float xScale = MAX_NAME_WIDTH / m_label->getContentWidth();

    float scale = std::min(xScale, yScale);
    m_label->setScale(scale);
    m_labelShadow->setScale(scale);

    m_labelContainer->setScaledContentSize(m_label->getScaledContentSize());

    if (m_labelButton) {
        m_labelButton->removeFromParent();
    }

    m_labelButton = Build(*m_labelContainer)
                        .intoMenuItem([this](auto btn) { this->onClick(btn); })
                        .scaleMult(1.1f)
                        .enabled(false)
                        .parent(this);

    this->updateSelfWidth();
}

void NameLabel::updateSelfWidth()
{
    this->updateLayout();
}

void NameLabel::onClick(CCMenuItemSpriteExtra *btn)
{
    if (m_callback) {
        m_callback(btn);
    }
}

void NameLabel::updateTeam(size_t idx, ccColor4B color)
{
    m_teamColor = color;
    m_teamIdx = idx;
    this->updateLabelColors();
}

void NameLabel::updateNoTeam()
{
    m_teamColor = std::nullopt;
    this->updateLabelColors();
}

void NameLabel::updateWithRoles(const SpecialUserData &data)
{
    this->removeAllBadges();

    bool colorSet = false;

    if (data.nameColor) {
        this->updateColor(*data.nameColor);
        colorSet = true;
    }

    for (auto id : data.roleIds) {
        // set name color to the highest prio role, if not overridden
        // server guarantees that the roles are sorted by priority
        auto role = NetworkManagerImpl::get().findRole(id);
        if (!role) {
            log::warn("Role ID {} not found when updating NameLabel badges", id);
            continue;
        }

        if (role->hide) {
            continue;
        }

        if (!colorSet) {
            this->updateColor(role->nameColor);
            colorSet = true;
        }

        auto badge = createBadge(id);

        if (!badge) {
            log::warn("Failed to create badge for role ID {}", id);
            continue;
        }

        m_badgeContainer->addChild(badge);

        if (!m_multipleBadges) {
            break;
        }
    }

    this->resizeBadgeContainer();
}

void NameLabel::updateNoRoles()
{
    this->removeAllBadges();
    this->updateColor({255, 255, 255});
    this->resizeBadgeContainer();
}

void NameLabel::addBadge(cocos2d::CCSprite *badge)
{
    m_badgeContainer->addChild(badge);

    this->resizeBadgeContainer();
}

void NameLabel::removeAllBadges()
{
    for (auto child : m_badgeContainer->getChildrenExt()) {
        if (child == m_teamLabel)
            continue;
        child->removeFromParent();
    }
}

void NameLabel::resizeBadgeContainer()
{
    size_t elems = 0;
    float width = 0.f;

    for (auto elem : m_badgeContainer->getChildrenExt()) {
        if (!elem->isVisible())
            continue;
        elems++;
        width += elem->getScaledContentWidth();
    }

    width += (elems - 1) * 3.f;

    if (elems == 0) {
        width = 0.f;
    }

    m_badgeContainer->setContentWidth(width);
    m_badgeContainer->updateLayout();

    this->updateSelfWidth();
}

void NameLabel::updateOpacity(float opacity)
{
    this->updateOpacity(static_cast<unsigned char>(opacity * 255.f));
}

void NameLabel::updateColor(MultiColor color)
{
    m_color = std::move(color);
    this->updateLabelColors();
}

void NameLabel::updateColor(const Color3 &color)
{
    this->updateColor(MultiColor::fromColor(color));
}

void NameLabel::updateLabelColors()
{
    if (!m_label)
        return;

    // team color always overrides the name color
    if (m_teamColor) {
        bool colorblind = globed::setting<bool>("core.ui.colorblind-mode");
        auto color = cue::into<ccColor3B>(*m_teamColor);

        if (colorblind) {
            if (!m_teamLabel) {
                m_teamLabel = Build<Label>::create("", "bigFont.fnt").parent(m_badgeContainer);
            }

            m_teamLabel->setString(fmt::to_string(m_teamIdx + 1));
            m_teamLabel->setColor(color);

            this->resizeBadgeContainer();
        } else {
            m_label->setColor(color);
            m_label->setOpacity(m_teamColor->a);
        }

        return;
    }

    if (m_teamLabel)
        m_teamLabel->setVisible(false);

    if (m_color.isGradient()) {
        m_label->setGradientColors(m_color);
    } else if (m_color.isTint()) {
        m_color.animateNode(m_label);
    } else {
        m_label->setColor(m_color.getColor());
    }

    this->resizeBadgeContainer();
}

void NameLabel::updateOpacity(unsigned char opacity)
{
    if (m_label) {
        m_label->setOpacity(opacity);
        m_labelShadow->setOpacity(opacity * 0.75f);
    }

    for (auto child : m_badgeContainer->getChildrenExt<CCSprite>()) {
        child->setOpacity(opacity);
    }
}

void NameLabel::makeClickable(std23::move_only_function<void(CCMenuItemSpriteExtra *)> callback)
{
    m_callback = std::move(callback);
    m_labelButton->setEnabled(true);
}

void NameLabel::setMultipleBadges(bool multiple)
{
    m_multipleBadges = multiple;

    // TODO (low) impl
}

void NameLabel::setShadowEnabled(bool enabled)
{
    m_shadow = enabled;

    if (m_labelShadow) {
        m_labelShadow->setVisible(enabled);
    }
}

NameLabel *NameLabel::create(const std::string &name, const char *font)
{
    auto ret = new NameLabel();
    if (ret->init(name, font)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

} // namespace globed