#pragma once
#include "../../prelude.hpp"

#include <Geode/Geode.hpp>
#ifdef GLOBED_BUILD
# include <ui/misc/LazyPlayerIcon.hpp>
#else
namespace cue {
struct Icons;
}
#endif

namespace globed {

class LazyPlayerIcon;

class GLOBED_DLL ProgressIcon : public CCNode {
public:
    ProgressIcon() = default;
    GLOBED_NOCOPY(ProgressIcon);
    GLOBED_NOMOVE(ProgressIcon);

    static ProgressIcon* create();

    void updateIcons(const cue::Icons& data);
    void updatePosition(float xpos, bool practicing);
    void toggleLine(bool enabled);
    void togglePracticeSprite(bool enabled);
    void setForceOnTop(bool state);

private:
    cocos2d::CCLayerColor* m_line = nullptr;
    LazyPlayerIcon* m_icon = nullptr;
    bool m_forceOnTop = false;

    bool init();
    void recalcOpacity();
};

}