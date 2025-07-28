#include "CountTriggerGameObject.hpp"
#include "../CounterChange.hpp"
#include "../Ids.hpp"


using namespace geode::prelude;

namespace globed {

void HookedCountObject::triggerObject(GJBaseGameLayer* layer, int idk, gd::vector<int> const* idunno) {
    if (m_objectID == 0x719 && globed::isWritableCustomItem(m_itemID)) {
        // gjbgl::addPickupTrigger reimpl
        auto gjbgl = GlobedGJBGL::get();

        CounterChange change{};
        change.itemId = m_itemID;

        using enum CounterChangeType;

        if (m_pickupTriggerMode == 1) {
            change.type = Multiply;
            change.value.asFloat() = m_pickupTriggerMultiplier;
        } else if (m_pickupTriggerMode == 2) {
            change.type = Divide;
            change.value.asFloat() = m_pickupTriggerMultiplier;
        } else if (!m_isOverride) {
            change.type = Add;
            change.value.asInt() = m_pickupCount;
        } else {
            change.type = Set;
            change.value.asInt() = m_pickupCount;
        }

        GlobalTriggersModule::get().queueCounterChange(change);
    } else if (!globed::isReadonlyCustomItem(m_itemID)) {
        CountTriggerGameObject::triggerObject(layer, idk, idunno);
    }
}

}
