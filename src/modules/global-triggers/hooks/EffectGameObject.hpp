#pragma once

#include <Geode/Geode.hpp>
#include <Geode/modify/EffectGameObject.hpp>
#include <globed/config.hpp>
#include "../GlobalTriggersModule.hpp"

namespace globed {

struct GLOBED_DLL GLOBED_NOVTABLE HookedEffectGameObject : geode::Modify<HookedEffectGameObject, EffectGameObject> {
    static void onModify(auto& self) {
        GLOBED_CLAIM_HOOKS(GlobalTriggersModule::get(), self,
            "EffectGameObject::triggerObject",
        );
    }

    $override
    void triggerObject(GJBaseGameLayer* layer, int idk, gd::vector<int> const* idunno);
};

}
