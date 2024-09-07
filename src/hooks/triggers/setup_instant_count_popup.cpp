#include "setup_instant_count_popup.hpp"
#include <globed/constants.hpp>

using namespace geode::prelude;

static bool inRange(int itemId) {
    return itemId >= globed::CUSTOM_ITEM_ID_START && itemId < globed::CUSTOM_ITEM_ID_END;
}

bool InstantCountPopupHook::init(CountTriggerGameObject* p0, cocos2d::CCArray* p1) {
    if (!SetupInstantCountPopup::init(p0, p1)) return false;

    return true;
}

void InstantCountPopupHook::updateItemID() {
    // TODO no mbo
#ifdef GEODE_IS_WINDOWS
    int itemId = *(int*)((uintptr_t)this + 0x3c0);
    if (!inRange(itemId)) {
        return SetupInstantCountPopup::updateItemID();
    }

    if (m_gameObject) {
        // bypass the checks
        m_gameObject->m_itemID = itemId;
        return;
    }

    if (m_gameObjects) {
        for (auto obj : CCArrayExt<EffectGameObject*>(m_gameObjects)) {
            obj->m_itemID = itemId;
        }
    }
#else
    return SetupInstantCountPopup::updateItemID();
#endif
}
