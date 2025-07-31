#include <Geode/Geode.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <globed/config.hpp>
#include <globed/core/actions.hpp>

using namespace geode::prelude;

namespace globed {

struct GLOBED_MODIFY_ATTR HookedLevelInfoLayer : geode::Modify<HookedLevelInfoLayer, LevelInfoLayer> {
    $override
    bool init(GJGameLevel* level, bool challenge) {
        if (!LevelInfoLayer::init(level, challenge)) {
            return false;
        }

        return true;
    }

    $override
    void onBack(CCObject* sender) {
        // clear warp context
        if (globed::_getWarpContext().levelId() == m_level->m_levelID) {
            globed::_clearWarpContext();
        }

        LevelInfoLayer::onBack(sender);
    }
};

}