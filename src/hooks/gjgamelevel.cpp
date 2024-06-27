#include "gjgamelevel.hpp"
#include <Geode/loader/Dispatch.hpp>
#include <hooks/gjbasegamelayer.hpp>

using namespace geode::prelude;

#ifndef GEODE_IS_ARM_MAC
void HookedGJGameLevel::savePercentage(int p0, bool p1, int p2, int p3, bool p4) {
    auto* gpl = GlobedGJBGL::get();
    if (!gpl || !gpl->m_fields->shouldStopProgress) {
        GJGameLevel::savePercentage(p0, p1, p2, p3, p4);
    }
}
#endif

LevelId HookedGJGameLevel::getLevelIDFrom(GJGameLevel* level) {
    using LevelIdEvent = DispatchEvent<GJGameLevel*, LevelId*>;
    using LevelIdFilter = DispatchFilter<GJGameLevel*, LevelId*>;

    LevelId levelId = level->m_levelID;

    LevelIdEvent("setup-level-id"_spr, level, &levelId).post();

    return levelId;
}
