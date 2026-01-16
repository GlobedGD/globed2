#pragma once

#include <Geode/Geode.hpp>
#include <globed/core/data/SpecialUserData.hpp>
#include <ui/misc/GradientLabel.hpp>

#include <std23/move_only_function.h>

namespace globed {

class NameLabel : public cocos2d::CCMenu {
public:
    static NameLabel *create(const std::string &name, const char *font = "chatFont.fnt");

    void makeClickable(std23::move_only_function<void(CCMenuItemSpriteExtra *)> callback);
    void updateName(const std::string &name);
    void updateName(const char *name);
    void updateTeam(size_t idx, cocos2d::ccColor4B color);
    void updateNoTeam();
    void updateWithRoles(const SpecialUserData &data);
    void updateNoRoles();
    void updateOpacity(unsigned char opacity);
    void updateOpacity(float opacity);
    void updateColor(MultiColor color);
    void updateColor(const Color3 &color);
    void setMultipleBadges(bool multiple);
    void setShadowEnabled(bool enabled);
    void addBadge(cocos2d::CCSprite *badge);
    void removeAllBadges();
    void updateSelfWidth();

private:
    GradientLabel *m_label = nullptr;
    Label *m_labelShadow = nullptr;
    Label *m_teamLabel = nullptr;
    geode::Ref<cocos2d::CCNode> m_labelContainer = nullptr;
    CCMenuItemSpriteExtra *m_labelButton = nullptr;
    cocos2d::CCNode *m_badgeContainer = nullptr;
    std23::move_only_function<void(CCMenuItemSpriteExtra *)> m_callback;
    const char *m_font = nullptr;
    bool m_multipleBadges = false;
    bool m_shadow = true;
    MultiColor m_color;
    std::optional<cocos2d::ccColor4B> m_teamColor;
    size_t m_teamIdx = 0;

    bool init(const std::string &name, const char *font);
    void resizeBadgeContainer();
    void onClick(CCMenuItemSpriteExtra *btn);
    void updateLabelColors();
};

} // namespace globed