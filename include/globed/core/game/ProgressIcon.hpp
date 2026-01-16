#pragma once
#include <globed/prelude.hpp>

#include <Geode/Geode.hpp>
#ifdef GLOBED_BUILD
#include <cue/PlayerIcon.hpp>
#else
namespace cue {

struct Icons;
class PlayerIcon;

} // namespace cue
#endif

namespace globed {

class GLOBED_DLL ProgressIcon : public CCNode {
public:
    ProgressIcon() = default;
    GLOBED_NOCOPY(ProgressIcon);
    GLOBED_NOMOVE(ProgressIcon);

    static ProgressIcon *create();

    void updateIcons(const cue::Icons &data);
    void updatePosition(float xpos, bool practicing);
    void toggleLine(bool enabled);
    void togglePracticeSprite(bool enabled);
    void setForceOnTop(bool state);

private:
    cocos2d::CCLayerColor *m_line = nullptr;
    cue::PlayerIcon *m_icon = nullptr;
    bool m_forceOnTop = false;

    bool init();
    void recalcOpacity();
};

} // namespace globed