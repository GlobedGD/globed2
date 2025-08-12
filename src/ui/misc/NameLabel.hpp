#pragma once

#include <Geode/Geode.hpp>

namespace globed {

class NameLabel : public cocos2d::CCMenu {
public:
    static NameLabel* create(const std::string& name, bool bigFont = false);

    void makeClickable(std::function<void(CCMenuItemSpriteExtra*)> callback);
    void updateName(const std::string& name);
    void updateName(const char* name);
    void updateTeam(size_t idx, cocos2d::ccColor4B color);
    void updateOpacity(unsigned char opacity);
    void updateOpacity(float opacity);
    void setMultipleBadges(bool multiple);
    void setShadowEnabled(bool enabled);

private:
    cocos2d::CCLabelBMFont* m_label = nullptr;
    cocos2d::CCLabelBMFont* m_labelShadow = nullptr;
    cocos2d::CCLabelBMFont* m_teamLabel = nullptr;
    cocos2d::CCNode* m_labelContainer = nullptr;
    CCMenuItemSpriteExtra* m_labelButton = nullptr;
    cocos2d::CCNode* m_badgeContainer = nullptr;
    std::function<void(CCMenuItemSpriteExtra*)> m_callback;
    bool m_bigFont;
    bool m_multipleBadges = false;
    bool m_shadow = true;

    bool init(const std::string& name, bool bigFont);
    void resizeBadgeContainer();
    void onClick(CCMenuItemSpriteExtra* btn);
};

}