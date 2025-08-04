#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <globed/config.hpp>
#include <globed/util/scary.hpp>
#include <modules/scripting/ScriptingModule.hpp>
#include <modules/scripting/objects/FireServerObject.hpp>
#include <modules/scripting/objects/Ids.hpp>

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
        auto [iobj, ty] = classifyObject(p0);
        if (ty != ScriptObjectType::None) {
            this->addScriptObject(iobj, ty);
            return;
        } else {
            PlayLayer::addObject(p0);
        }
    }

    void addScriptObject(ItemTriggerGameObject* original, ScriptObjectType type) {
        GameObject* obj = original;

        switch (type) {
            case ScriptObjectType::FireServer: {
                obj = vtable_cast<FireServerObject*>(original);
            } break;

            default: {
                log::warn("Ignoring unknown script object type: {}", (int)type);
                return;
            } break;
        }

        PlayLayer::addObject(obj);
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
