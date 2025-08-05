#pragma once

#include <Geode/Geode.hpp>

namespace globed {

// vanilla object ids
struct ObjectId {
    typedef enum {
        ItemEditTrigger = 3619,
        ItemCompareTrigger = 3620,
    } Id;

    Id id;

    inline ObjectId(Id id) : id(id) {}

    operator int() const {
        return (int)id;
    }
};

// custom object ids
constexpr inline uint32_t SCRIPT_OBJECT_IDENT_MASK = 0x71eb0000; // top bit must be 0 :)
constexpr inline uint32_t SCRIPT_OBJECT_TYPE_MASK = 0x0000ffff;

enum class ScriptObjectType : uint32_t {
    None = 0x0,
    FireServer = 0x1,
};

inline std::pair<ItemTriggerGameObject*, ScriptObjectType> classifyObject(GameObject* in) {
    if (!in || in->m_objectID != ObjectId::ItemCompareTrigger) {
        return {nullptr, ScriptObjectType::None};
    }

    auto obj = geode::cast::typeinfo_cast<ItemTriggerGameObject*>(in);
    if (!obj) {
#ifdef GLOBED_DEBUG
        geode::log::warn("Object {} is not of the right type", in);
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

inline void setObjectAttrs(ItemTriggerGameObject* obj, ScriptObjectType type) {
    obj->m_resultType3 = SCRIPT_OBJECT_IDENT_MASK | static_cast<uint32_t>(type);
}

inline const char* textureForScriptObject(ScriptObjectType type) {
    switch (type) {
        case ScriptObjectType::FireServer:
            return "trigger-fire-server.png"_spr;
        default:
            return "globed-gold-icon.png"_spr;
    }
}

inline void setObjectTexture(GameObject* obj, ScriptObjectType type) {
    auto tex = cocos2d::CCTextureCache::get()->addImage(textureForScriptObject(type), false);
    if (!tex) {
        geode::log::error("Failed to load texture for script object type {}", (int)type);
        return;
    }

    obj->setTexture(tex);

    cocos2d::CCRect rect{};
    rect.size = tex->getContentSize();
    obj->setTextureRect(rect, false, rect.size);
}

}
