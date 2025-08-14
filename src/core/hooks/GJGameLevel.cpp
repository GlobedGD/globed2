#include "GJBaseGameLayer.hpp"
#include <Geode/modify/GJGameLevel.hpp>

using namespace geode::prelude;

namespace globed {

struct GLOBED_MODIFY_ATTR HookedGJGameLevel : geode::Modify<HookedGJGameLevel, GJGameLevel> {
#ifndef GEODE_IS_ARM_MAC
    $override
    void savePercentage(int p0, bool p1, int p2, int p3, bool p4) {
        auto* gpl = GlobedGJBGL::get();
        if (!gpl || !gpl->isSafeMode()) {
            GJGameLevel::savePercentage(p0, p1, p2, p3, p4);
        }
    }
#endif
};

}
