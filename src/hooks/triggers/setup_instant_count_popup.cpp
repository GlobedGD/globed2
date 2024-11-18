#include "setup_instant_count_popup.hpp"

#ifdef GLOBED_GP_CHANGES

#include <globed/constants.hpp>

using namespace geode::prelude;

bool InstantCountPopupHook::init(CountTriggerGameObject* p0, cocos2d::CCArray* p1) {
    if (!SetupInstantCountPopup::init(p0, p1)) return false;

    return true;
}

void InstantCountPopupHook::updateItemID() {
    int itemId = m_itemID;

    if (!globed::isCustomItem(itemId)) {
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
    return SetupInstantCountPopup::updateItemID();
}

#endif // GLOBED_GP_CHANGES
