#pragma once

#include <Geode/Geode.hpp>
#include <Geode/modify/EffectGameObject.hpp>
#include <globed/config.hpp>
#include "../GlobalTriggersModule.hpp"

namespace globed {

struct GLOBED_MODIFY_ATTR HookedEffectGameObject : geode::Modify<HookedEffectGameObject, EffectGameObject> {
    static void onModify(auto& self) {
        GlobalTriggersModule::get().claimHooks(self);
    }

    $override
    void triggerObject(GJBaseGameLayer* layer, int idk, gd::vector<int> const* idunno);
};

}
