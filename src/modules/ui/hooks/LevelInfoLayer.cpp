#include <Geode/Geode.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <globed/config.hpp>
#include <globed/core/actions.hpp>
#include <modules/ui/UIModule.hpp>

using namespace geode::prelude;

namespace globed {

struct GLOBED_MODIFY_ATTR HookedLevelInfoLayer : geode::Modify<HookedLevelInfoLayer, LevelInfoLayer> {
    static void onModify(auto& self) {
        GLOBED_CLAIM_HOOKS(UIModule::get(), self,
            "LevelInfoLayer::init",
            "LevelInfoLayer::onBack",
        );
    }

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