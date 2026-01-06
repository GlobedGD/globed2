#include "Common.hpp"
#include <Geode/modify/EditLevelLayer.hpp>
#include <modules/ui/UIModule.hpp>

#include <globed/config.hpp>
#include <Geode/Geode.hpp>

using namespace geode::prelude;

namespace globed {

struct GLOBED_MODIFY_ATTR HookedEditLevelLayer : geode::Modify<HookedEditLevelLayer, EditLevelLayer> {
    static void onModify(auto& self) {
        GLOBED_CLAIM_HOOKS(UIModule::get(), self,
            "EditLevelLayer::onEdit",
            "EditLevelLayer::onPlay",
        );

        (void) self.setHookPriority("EditLevelLayer::onEdit", -100);
        (void) self.setHookPriority("EditLevelLayer::onPlay", -100);
    }

    void onEdit(CCObject* p) {
        if (disallowLevelJoin(-1)) {
            return;
        }

        EditLevelLayer::onEdit(p);
    }

    void onPlay(CCObject* p) {
        if (disallowLevelJoin(-1)) {
            return;
        }

        EditLevelLayer::onPlay(p);
    }
};

}
