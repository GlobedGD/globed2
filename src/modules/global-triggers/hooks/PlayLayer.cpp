#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <globed/config.hpp>
#include "../Ids.hpp"
#include "../GlobalTriggersModule.hpp"
#include "GJBaseGameLayer.hpp"

using namespace geode::prelude;

namespace globed {

struct GLOBED_NOVTABLE GLOBED_DLL GTPlayLayerHook : geode::Modify<GTPlayLayerHook, PlayLayer> {
    static void onModify(auto& self) {
        GLOBED_CLAIM_HOOKS(GlobalTriggersModule::get(), self,
            "PlayLayer::setupHasCompleted",
        );
    }

    void setupHasCompleted() {
        PlayLayer::setupHasCompleted();

        // detect if the level has any global triggers
        bool hasGlobalTriggers = false;

        for (auto obj : CCArrayExt<GameObject>(m_objects)) {
            if (obj->m_classType != GameObjectClassType::Effect) {
                continue;
            }

            auto ego = static_cast<EffectGameObject*>(obj);

            if (globed::isCustomItem(ego->m_itemID) || globed::isCustomItem(ego->m_itemID2) || globed::isCustomItem(ego->m_targetGroupID)) {
                log::info("Enabling custom items, level supports them!");
                hasGlobalTriggers = true;
                break;
            }
        }

        if (!hasGlobalTriggers) {
            log::debug("No global triggers, disabling module");
            (void) GlobalTriggersModule::get().disable();
            return;
        }

        auto layer = GTriggersGJBGL::get(this);
        layer->postInit();
    }
};

}
