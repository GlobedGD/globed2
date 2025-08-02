#pragma once

#include <Geode/Geode.hpp>

namespace globed {

class NameLabel : public cocos2d::CCNode {
public:
    static NameLabel* create(const std::string& name, bool bigFont = false);

    void updateData(const std::string& name);
    void updateName(const std::string& name);
    void updateName(const char* name);
    void updateOpacity(unsigned char opacity);
    void updateOpacity(float opacity);
    void setMultipleBadges(bool multiple);

private:
    cocos2d::CCLabelBMFont* m_label = nullptr;
    cocos2d::CCLabelBMFont* m_labelShadow = nullptr;
    cocos2d::CCNode* m_labelContainer = nullptr;
    cocos2d::CCNode* m_badgeContainer = nullptr;
    bool m_bigFont;
    bool m_multipleBadges = false;

    bool init(const std::string& name, bool bigFont);
};

}