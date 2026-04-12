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

struct GameCameraState;
class LazyPlayerIcon;

class GLOBED_DLL ProgressArrow : public CCNode {
public:
    ProgressArrow() = default;
    GLOBED_NOCOPY(ProgressArrow);
    GLOBED_NOMOVE(ProgressArrow);

    static ProgressArrow* create();

    void updateIcons(const cue::Icons& data);
    void updatePosition(const GameCameraState& cam, CCPoint playerPos, double angle);

private:
    LazyPlayerIcon* m_icon = nullptr;

    bool init();
};

}