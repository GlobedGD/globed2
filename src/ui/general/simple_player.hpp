#pragma once
#include <defs/geode.hpp>

#include <data/types/gd.hpp>

class GlobedSimplePlayer : public cocos2d::CCNode, public cocos2d::CCRGBAProtocol {
public:
    struct Icons {
        IconType type;
        int id;
        int color1, color2, color3; // color3 is -1 for glow disabled
    };

    static GlobedSimplePlayer* create(const Icons& icons);
    static GlobedSimplePlayer* create(const PlayerIconData& icons);
    static GlobedSimplePlayer* create(IconType type, const PlayerIconData& icons);

    void updateIcons(const Icons& icons);
    SimplePlayer* getInner();

    void setColor(const cocos2d::ccColor3B& color) { sp->setColor(color); }
    const cocos2d::ccColor3B& getColor() { return sp->getColor(); }
    const cocos2d::ccColor3B& getDisplayedColor() { return sp->getDisplayedColor(); }
    GLubyte getDisplayedOpacity() { return sp->getDisplayedOpacity(); }
    GLubyte getOpacity() { return sp->getOpacity(); }
    void setOpacity(GLubyte opacity) { sp->setOpacity(opacity); }
    void setOpacityModifyRGB(bool bValue) { sp->setOpacityModifyRGB(bValue); }
    bool isOpacityModifyRGB() { return sp->isOpacityModifyRGB(); }
    bool isCascadeColorEnabled() { return sp->isCascadeColorEnabled(); }
    void setCascadeColorEnabled(bool cascadeColorEnabled) { sp->setCascadeColorEnabled(cascadeColorEnabled); }
    void updateDisplayedColor(const cocos2d::ccColor3B& color) { sp->updateDisplayedColor(color); }
    bool isCascadeOpacityEnabled() { return sp->isCascadeOpacityEnabled(); }
    void setCascadeOpacityEnabled(bool cascadeOpacityEnabled) { sp->setCascadeOpacityEnabled(cascadeOpacityEnabled); }
    void updateDisplayedOpacity(GLubyte opacity) { sp->updateDisplayedOpacity(opacity); }

    SimplePlayer* sp;

protected:
    bool init(const Icons& icons);
    void updateIcons();

    Icons icons;
};
