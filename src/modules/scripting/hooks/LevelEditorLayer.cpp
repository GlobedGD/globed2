#include <Geode/Geode.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/GameManager.hpp>
#include <globed/config.hpp>
#include <globed/util/scary.hpp>
#include <globed/util/assert.hpp>
#include <modules/scripting/ScriptingModule.hpp>
#include <modules/scripting/objects/FireServerObject.hpp>
#include <modules/scripting/objects/Ids.hpp>

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
            auto [iobj, ty] = classifyObject(obj);

            if (ty != ScriptObjectType::None) {
                GLOBED_ASSERT(iobj);

                this->addScriptObjectInplace(iobj, ty);
            }
        }
    }

    void addScriptObjectInplace(ItemTriggerGameObject* original, ScriptObjectType type) {
        switch (type) {
            case ScriptObjectType::FireServer: {
                (void) vtable_cast<FireServerObject*>(original);
            } break;

            default: {
                log::warn("Ignoring unknown script object type: {}", (int)type);
                return;
            } break;
        }

        setObjectTexture(original, type);
    }

    $override
    GameObject* createObject(int objectIdRaw, CCPoint p1, bool p2) {
        uint32_t objectId = objectIdRaw;

        if ((objectId & ~SCRIPT_OBJECT_TYPE_MASK) == SCRIPT_OBJECT_IDENT_MASK) {
            ScriptObjectType type = static_cast<ScriptObjectType>(objectId & SCRIPT_OBJECT_TYPE_MASK);
            return this->onCustomCreateObject(type, p1, p2);
        } else {
            auto obj =  LevelEditorLayer::createObject(objectIdRaw, p1, p2);
            return obj;
        }
    }

    GameObject* onCustomCreateObject(ScriptObjectType type, CCPoint p1, bool p2) {
        auto obj = static_cast<ItemTriggerGameObject*>(LevelEditorLayer::createObject(ObjectId::ItemCompareTrigger, p1, p2));

        setObjectAttrs(obj, type);
        setObjectTexture(obj, type);

        switch (type) {
            case ScriptObjectType::FireServer: {
                auto object = vtable_cast<FireServerObject*>(obj);

                // TODO: temporary payload
                // object->encodePayload({});
                object->encodePayload({
                    .eventId = 123,
                    .argCount = 3,
                    .args = {{
                        FireServerArg{
                            .type = FireServerArgType::Static,
                            .value = 42,
                        },
                        FireServerArg{
                            .type = FireServerArgType::Item,
                            .value = 2,
                        },
                        FireServerArg{
                            .type = FireServerArgType::Timer,
                            .value = 3,
                        }
                    }},
                });
            } break;

            default: break;
        }

        return obj;
    }
};

}
