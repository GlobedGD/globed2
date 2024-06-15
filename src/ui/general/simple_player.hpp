#pragma once
#include <defs/geode.hpp>

#include <data/types/gd.hpp>

class GlobedSimplePlayer : public cocos2d::CCNode, public cocos2d::CCRGBAProtocol {
public:
    struct Icons {
        IconType type;
        int id;
        int color1, color2, color3; // color3 is -1 for glow disabled

        // jesus christ
        Icons() : type(IconType::Cube), id(1), color1(1), color2(3), color3(NO_GLOW) {}
        Icons(IconType type, int id, int color1, int color2, int color3) : type(type), id(id), color1(color1), color2(color2), color3(color3) {}
        Icons(int id, int color1, int color2, int color3) : Icons(IconType::Cube, id, color1, color2, color3) {}
        Icons(const PlayerIconData& icons) : type(IconType::Cube), id(icons.cube), color1(icons.color1), color2(icons.color2), color3(icons.glowColor == NO_GLOW ? -1 : icons.glowColor) {}
        Icons(const PlayerIconDataSimple& icons) : type(IconType::Cube), id(icons.cube), color1(icons.color1), color2(icons.color2), color3(icons.glowColor == NO_GLOW ? -1 : icons.glowColor) {}
        Icons(const PlayerAccountData& data) : Icons(data.icons) {}
        Icons(const PlayerPreviewAccountData& data) : Icons(data.icons) {}
        Icons(const PlayerRoomPreviewAccountData& data) : Icons(data.icons) {}
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
