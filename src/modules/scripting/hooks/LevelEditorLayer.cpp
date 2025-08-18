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
            std::optional<EmbeddedScript> script;
            globed::onAddObject(obj, true, script);
        }
    }

    $override
    GameObject* createObject(int p0, cocos2d::CCPoint p1, bool p2) {
        auto obj = LevelEditorLayer::createObject(p0, p1, p2);

        auto type = classifyObjectId(p0);
        if (type != ScriptObjectType::None) {
            globed::postCreateObject(obj, type);
        }

        return obj;
    }

    $override
    static void updateObjectLabel(GameObject* obj) {
        LevelEditorLayer::updateObjectLabel(obj);

        globed::onUpdateObjectLabel(obj);
    }
};

}
