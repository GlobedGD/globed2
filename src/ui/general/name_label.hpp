#pragma once
#include <defs/geode.hpp>
#include <data/types/gd.hpp>

/* Container for username and badge */
class GlobedNameLabel : public cocos2d::CCNode {
public:
    void updateData(const std::string& name, cocos2d::CCSprite* badgeSprite, const RichColor& nameColor);
    void updateData(const std::string& name, std::optional<SpecialUserData> sud);
    void updateBadge(cocos2d::CCSprite* badgeSprite);
    void updateName(const std::string& name);
    void updateName(const char* name);
    void updateOpacity(float opacity);
    void updateOpacity(unsigned char opacity);
    void updateColor(const RichColor& color);

    static GlobedNameLabel* create(const std::string& name, cocos2d::CCSprite* badgeSprite, const RichColor& nameColor);
    static GlobedNameLabel* create(const std::string& name, std::optional<SpecialUserData> sud);
    static GlobedNameLabel* create(const std::string& name);

private:
    cocos2d::CCSprite* badge = nullptr;
    cocos2d::CCLabelBMFont* label = nullptr;
    constexpr static int COLOR_ACTION_TAG = 3498567;

    bool init(const std::string& name, cocos2d::CCSprite* badgeSprite, const RichColor& nameColor);
};