#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <globed/config.hpp>
#include <globed/util/scary.hpp>
#include <modules/scripting/ScriptingModule.hpp>
#include <modules/scripting/objects/FireServerObject.hpp>
#include <modules/scripting/objects/Ids.hpp>
#include "Common.hpp"


using namespace geode::prelude;

namespace globed {

struct GLOBED_NOVTABLE GLOBED_DLL SCPlayLayerHook : geode::Modify<SCPlayLayerHook, PlayLayer> {
    struct Fields {
        bool m_hasScriptObjects = false;
    };

    static void onModify(auto& self) {
        (void) self.setHookPriority("PlayLayer::addObject", -1000);

        GLOBED_CLAIM_HOOKS(ScriptingModule::get(), self,
            "PlayLayer::setupHasCompleted",
            "PlayLayer::addObject",
        );
    }

    $override
    void addObject(GameObject* p0) {
        if (globed::onAddObject(p0, false)) {
            m_fields->m_hasScriptObjects = true;
        }

        PlayLayer::addObject(p0);
    }

    $override
    void setupHasCompleted() {
        PlayLayer::setupHasCompleted();

        if (!m_fields->m_hasScriptObjects) {
            log::debug("No script objects, disabling module");
            (void) ScriptingModule::get().disable();
            return;
        }
    }
};

}
