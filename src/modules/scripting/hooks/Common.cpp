#include "Common.hpp"
#include <globed/util/scary.hpp>
#include <modules/scripting-ui/ui/SetupFireServerPopup.hpp>
#include <modules/scripting-ui/ui/SetupEmbeddedScriptPopup.hpp>
#include <modules/scripting/objects/ExtendedObjectBase.hpp>
#include <modules/scripting/objects/FireServerObject.hpp>
#include <modules/scripting/objects/ListenEventObject.hpp>

using namespace geode::prelude;

namespace globed {

static void setObjectTexture(GameObject* obj, ScriptObjectType type) {
    auto tex = CCTextureCache::get()->addImage(textureForScriptObject(type), false);
    if (!tex) {
        geode::log::error("Failed to load texture for script object type {}", (int)type);
        return;
    }

    if (type == ScriptObjectType::EmbeddedScript) {
        obj->m_hasSpecialChild = true;
        obj->removeAllChildren();

        auto spr = CCSprite::createWithTexture(tex);
        spr->setPosition(spr->getScaledContentSize() / 2.f);
        obj->setContentSize(spr->getScaledContentSize());
        obj->addChild(spr);
    } else {
        obj->setTexture(tex);

        CCRect rect{};
        rect.size = tex->getContentSize();
        obj->setTextureRect(rect, false, rect.size);
        obj->setContentSize(rect.size);
    }
}

static std::pair<GameObject*, ScriptObjectType> classifyObject(GameObject* in) {
    if (!in) {
        return {nullptr, ScriptObjectType::None};
    }

    if (in->m_objectID == ObjectId::Label) {
        auto obj = typeinfo_cast<TextGameObject*>(in);
        if (!obj) {
#ifdef GLOBED_DEBUG
            log::warn("Object {} is not of the right type (expected TextGameObject)", in);
#endif
            return {nullptr, ScriptObjectType::None};;
        }

        if (EmbeddedScript::decodeHeader(std::span{(uint8_t*)obj->m_text.data(), obj->m_text.size()})) {
            return {obj, ScriptObjectType::EmbeddedScript};
        }

        return {nullptr, ScriptObjectType::None};
    } else if (in->m_objectID != ObjectId::ItemCompareTrigger) {
        return {nullptr, ScriptObjectType::None};
    }

    auto obj = typeinfo_cast<ItemTriggerGameObject*>(in);
    if (!obj) {
#ifdef GLOBED_DEBUG
        log::warn("Object {} is not of the right type", in);
#endif
        return {nullptr, ScriptObjectType::None};
    }

    uint32_t ident = obj->m_resultType3;

    // script objects are identified by the top 16 bits of the result type having a specific value
    if ((ident & 0xffff0000) == SCRIPT_OBJECT_IDENT_MASK) {
        uint32_t type = ident & SCRIPT_OBJECT_TYPE_MASK;
        return {obj, (ScriptObjectType)type};
    } else {
        // a regular item compare trigger
        return {nullptr, ScriptObjectType::None};
    }
}

ScriptObjectType classifyObjectId(int objectIdRaw) {
    uint32_t objectId = objectIdRaw;

    if ((objectId & ~SCRIPT_OBJECT_TYPE_MASK) == SCRIPT_OBJECT_IDENT_MASK) {
        return static_cast<ScriptObjectType>(objectId & SCRIPT_OBJECT_TYPE_MASK);
    }

    return ScriptObjectType::None;
}

bool onAddObject(GameObject* original, bool editor, std::optional<EmbeddedScript>& outScript) {
    auto [iobj, ty] = classifyObject(original);

    if (ty == ScriptObjectType::None) {
        return false;
    }

    switch (ty) {
        case ScriptObjectType::FireServer: {
            (void) vtable_cast<FireServerObject*>(original);
        } break;

        case ScriptObjectType::EmbeddedScript: {
            if (auto obj = typeinfo_cast<TextGameObject*>(original)) {
                auto& str = obj->m_text;

                if (auto res = EmbeddedScript::decode(std::span{(uint8_t*) str.data(), str.size()})) {
                    outScript = *res;
                } else {
                    log::warn("Failed to decode embedded script: {}", res.unwrapErr());
                }

                original->m_hasSpecialChild = true; // force it to be in the ccnodecontainer instead of batch node, thank you alpha :)
            }
        } break;

        // case ScriptObjectType::ListenEvent: {
        //     (void) vtable_cast<ListenEventObject*>(original);
        // } break;

        default: {
            log::warn("Ignoring unknown script object type: {}", (int)ty);
            return false;
        } break;
    }

    if (editor) {
        setObjectTexture(original, ty);
    }

    return true;
}

bool onEditObject(GameObject* obj) {
#ifndef GLOBED_MODULE_SCRIPTING_UI
    return false;
#else
    auto [iobj, type] = classifyObject(obj);
    if (type == ScriptObjectType::None) {
        return false;
    }

    switch (type) {
        case ScriptObjectType::FireServer: {
            SetupFireServerPopup::create(static_cast<FireServerObject*>(obj))->show();
        } break;

        case ScriptObjectType::EmbeddedScript: {
            SetupEmbeddedScriptPopup::create(static_cast<TextGameObject*>(obj))->show();
        } break;

        default: {
            log::warn("Cannot edit unknown script object type: {}", (int)type);
        } break;
    }

    return true;
#endif
}

static void setObjectAttrs(ItemTriggerGameObject* obj, ScriptObjectType type) {
    obj->m_resultType3 = SCRIPT_OBJECT_IDENT_MASK | static_cast<uint32_t>(type);
}

const char* textureForScriptObject(ScriptObjectType type) {
    switch (type) {
        case ScriptObjectType::FireServer:
            return "trigger-fire-server.png"_spr;
        case ScriptObjectType::ListenEvent:
            return "trigger-listen-event.png"_spr;
        case ScriptObjectType::EmbeddedScript:
            return "trigger-server-script.png"_spr;
        default:
            return "globed-gold-icon.png"_spr;
    }
}

// void onCreateObject(ItemTriggerGameObject* obj, ScriptObjectType type) {
//     setObjectAttrs(obj, type);
//     setObjectTexture(obj, type);

//     switch (type) {
//         case ScriptObjectType::FireServer: {
//             auto object = vtable_cast<FireServerObject*>(obj);
//             object->encodePayload({});
//         } break;

//         // case ScriptObjectType::ListenEvent: {
//         //     auto object = vtable_cast<ListenEventObject*>(obj);
//         //     object->encodePayload({0, 0});
//         // } break;

//         default: break;
//     }
// }

void postCreateObject(GameObject* obj, ScriptObjectType type) {
    setObjectTexture(obj, type);
}

void onUpdateObjectLabel(GameObject* obj) {
    auto [iobj, type] = classifyObject(obj);
    if (!iobj || type == ScriptObjectType::None) return;

    setObjectTexture(iobj, type);
}

}