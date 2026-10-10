#pragma once

#include <Geode/Geode.hpp>
#include <Geode/modify/CountTriggerGameObject.hpp>
#include <globed/config.hpp>
#include "../GlobalTriggersModule.hpp"

namespace globed {

// gjbgl collectedObject and addCountToItem inlined on windows.

struct GLOBED_MODIFY_ATTR HookedCountObject : geode::Modify<HookedCountObject, CountTriggerGameObject> {
    static void onModify(auto& self) {
        GlobalTriggersModule::get().claimHooks(self);
    }

    $override
    void triggerObject(GJBaseGameLayer* layer, int idk, gd::vector<int> const* idunno);
};

}
