#include "gjgamelevel.hpp"
#include <hooks/play_layer.hpp>

void HookedGJGameLevel::savePercentage(int p0, bool p1, int p2, int p3, bool p4) {
    auto* gpl = static_cast<GlobedPlayLayer*>(PlayLayer::get());
    if (!gpl || !gpl->m_fields->shouldStopProgress) {
        GJGameLevel::savePercentage(p0, p1, p2, p3, p4);
    }
}
