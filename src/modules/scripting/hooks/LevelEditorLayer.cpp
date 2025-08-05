#include <globed/config.hpp>
#include <globed/util/assert.hpp>
#include <modules/scripting/ScriptingModule.hpp>
#include <modules/scripting/objects/FireServerObject.hpp>
#include <modules/scripting/objects/Ids.hpp>
#include "Common.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/GameManager.hpp>

using namespace geode::prelude;

namespace globed {

struct GLOBED_NOVTABLE GLOBED_DLL SCEditorHook : geode::Modify<SCEditorHook, LevelEditorLayer> {
    struct Fields {
        bool m_hasScriptObjects = false;
    };

    static void onModify(auto& self) {
        (void) self.setHookPriority("LevelEditorLayer::createObjectsFromString", 10000);

        // GLOBED_CLAIM_HOOKS(ScriptingModule::get(), self,
        //     "LevelEditorLayer::createObjectsFromString",
        // );
    }

    $override
    void createObjectsFromSetup(gd::string& p0) {
        LevelEditorLayer::createObjectsFromSetup(p0);

        for (auto obj : CCArrayExt<GameObject>(m_objects)) {
            globed::onAddObject(obj, true);
        }
    }

    $override
    GameObject* createObject(int objectIdRaw, CCPoint p1, bool p2) {
        auto type = globed::classifyObjectId(objectIdRaw);

        if (type != ScriptObjectType::None) {
            return this->onCustomCreateObject(type, p1, p2);
        } else {
            return LevelEditorLayer::createObject(objectIdRaw, p1, p2);
        }
    }

    GameObject* onCustomCreateObject(ScriptObjectType type, CCPoint p1, bool p2) {
        auto obj = static_cast<ItemTriggerGameObject*>(LevelEditorLayer::createObject(ObjectId::ItemCompareTrigger, p1, p2));

        globed::onCreateObject(obj, type);

        return obj;
    }

};

}
