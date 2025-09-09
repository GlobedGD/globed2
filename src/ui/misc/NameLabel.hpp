#pragma once

#include <Geode/Geode.hpp>
#include <globed/core/data/SpecialUserData.hpp>

namespace globed {

class NameLabel : public cocos2d::CCMenu {
public:
    static NameLabel* create(const std::string& name, const char* font = "chatFont.fnt", bool alignMiddle = false);

    void makeClickable(std::function<void(CCMenuItemSpriteExtra*)> callback);
    void updateName(const std::string& name);
    void updateName(const char* name);
    void updateTeam(size_t idx, cocos2d::ccColor4B color);
    void updateNoTeam();
    void updateWithRoles(const SpecialUserData& data);
    void updateOpacity(unsigned char opacity);
    void updateOpacity(float opacity);
    void updateColor(const MultiColor& color);
    void updateColor(const Color3& color);
    void setMultipleBadges(bool multiple);
    void setShadowEnabled(bool enabled);

private:
    cocos2d::CCLabelBMFont* m_label = nullptr;
    cocos2d::CCLabelBMFont* m_labelShadow = nullptr;
    cocos2d::CCLabelBMFont* m_teamLabel = nullptr;
    geode::Ref<cocos2d::CCNode> m_labelContainer = nullptr;
    CCMenuItemSpriteExtra* m_labelButton = nullptr;
    cocos2d::CCNode* m_badgeContainer = nullptr;
    std::function<void(CCMenuItemSpriteExtra*)> m_callback;
    const char* m_font = nullptr;
    bool m_multipleBadges = false;
    bool m_shadow = true;

    bool init(const std::string& name, const char* font, bool alignMiddle);
    void resizeBadgeContainer();
    void onClick(CCMenuItemSpriteExtra* btn);
};

}