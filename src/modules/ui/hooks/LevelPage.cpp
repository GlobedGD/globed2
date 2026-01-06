#include <globed/config.hpp>
#include <modules/ui/UIModule.hpp>
#include "Common.hpp"

#include <Geode/modify/LevelPage.hpp>

using namespace geode::prelude;

namespace globed {

struct GLOBED_MODIFY_ATTR HookedLevelPage : Modify<HookedLevelPage, LevelPage> {
    static void onModify(auto& self) {
        GLOBED_CLAIM_HOOKS(UIModule::get(), self,
            "LevelPage::onPlay",
            "LevelPage::onTheTower",
        );

        (void) self.setHookPriority("LevelPage::onPlay", -100);
    }

    $override
    void onPlay(CCObject* p) {
        if (disallowLevelJoin(m_level ? m_level->m_levelID : 0)) {
            return;
        }

        LevelPage::onPlay(p);
    }

    $override
    void onTheTower(CCObject* p) {
        if (disallowLevelJoin(-1)) {
            return;
        }

        LevelPage::onTheTower(p);
    }
};

}