#pragma once
#include <globed/prelude.hpp>

#include <Geode/Geode.hpp>
#ifdef GLOBED_BUILD
# include <cue/PlayerIcon.hpp>
#else
namespace cue {

struct Icons;
class PlayerIcon;

}
#endif

namespace globed {

struct GameCameraState;

class GLOBED_DLL ProgressArrow : public CCNode {
public:
    GLOBED_NOCOPY(ProgressArrow);
    GLOBED_NOMOVE(ProgressArrow);

    static ProgressArrow* create();

    void updateIcons(const cue::Icons& data);
    void updatePosition(const GameCameraState& cam, CCPoint playerPos, double angle);

private:
    cue::PlayerIcon* m_icon = nullptr;

    bool init();
};

}