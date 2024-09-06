#include "gjeffectmanager.hpp"

#include <Geode/modify/GJEffectManager.hpp>
#include <Geode/modify/EffectGameObject.hpp>
#include <Geode/modify/CountTriggerGameObject.hpp>
#include <hooks/gjbasegamelayer.hpp>
#include <defs/platform.hpp>
#include <defs/minimal_geode.hpp>
#include <globed/constants.hpp>

static bool inRange(int item) {
    return item >= globed::CUSTOM_ITEM_ID_START && item < globed::CUSTOM_ITEM_ID_END;
}

void GJEffectManagerHook::updateCountForItem(int id, int value) {
    if (inRange(id)) {
        this->updateCountForItemReimpl(id, value);
    } else {
        GJEffectManager::updateCountForItem(id, value);
    }
}

void GJEffectManagerHook::addCountToItemCustom(int id, int diff) {
    int newValue = m_fields->customItems[id] + diff;
    this->updateCountForItemReimpl(id, newValue);
}

void GJEffectManagerHook::updateCountForItemReimpl(int id, int value) {
    auto& fields = *m_fields.self();

    fields.customItems[id] = value;
}

int GJEffectManagerHook::countForItemCustom(int id) {
    return m_fields->customItems[id];
}

void GJEffectManagerHook::applyFromCounterChange(const GlobedCounterChange& change) {
    using enum GlobedCounterChange::Type;

    int prev = this->countForItemCustom(change.itemId);

    switch (change.type) {
        case Add: {
            this->updateCountForItemReimpl(change.itemId, prev + change._val.intVal);
        } break;
        case Set: {
            this->updateCountForItemReimpl(change.itemId, change._val.intVal);
        } break;
        case Multiply: {
            this->updateCountForItemReimpl(change.itemId, prev * change._val.floatVal);
        } break;
        case Divide: {
            this->updateCountForItemReimpl(change.itemId, prev / change._val.floatVal);
        } break;
    }

    GlobedGJBGL::get()->updateCounters(change.itemId, this->countForItemCustom(change.itemId));
}

// gjbgl collectedObject and addCountToItem inlined on windows.
struct GLOBED_DLL EffectGameObjectHook : geode::Modify<EffectGameObjectHook, EffectGameObject> {
    void triggerObject(GJBaseGameLayer* layer, int idk, gd::vector<int> const* idunno) {
        log::debug("item id: {}", m_itemID);
        if (this->m_collectibleIsPickupItem && inRange(m_itemID)) {
            log::debug("triggered for {}", m_itemID);
            auto em = static_cast<GJEffectManagerHook*>(layer->m_effectManager);

            if (m_subtractCount) {
                em->addCountToItemCustom(m_itemID, -1);
            } else {
                em->addCountToItemCustom(m_itemID, 1);
            }

            int count = em->m_fields->customItems[m_itemID];
            layer->updateCounters(m_itemID, count);
        } else {
            EffectGameObject::triggerObject(layer, idk, idunno);
        }
    }
};

// gjbgl collectedObject and addCountToItem inlined on windows.
struct GLOBED_DLL CountObjectHook : geode::Modify<CountObjectHook, CountTriggerGameObject> {
    void triggerObject(GJBaseGameLayer* layer, int idk, gd::vector<int> const* idunno) {
        if (m_objectID == 0x719 && inRange(m_itemID)) {
            // gjbgl::addPickupTrigger reimpl
            auto em = static_cast<GJEffectManagerHook*>(layer->m_effectManager);
            auto gjbgl = GlobedGJBGL::get();

            GlobedCounterChange cc;
            cc.itemId = m_itemID;

            using enum GlobedCounterChange::Type;

            if (m_pickupTriggerMode == 1) {
                auto count = em->countForItemCustom(m_itemID);
                em->updateCountForItemReimpl(m_itemID, count * m_pickupTriggerMultiplier);
                cc.type = Multiply;
                cc._val.floatVal = m_pickupTriggerMultiplier;
            } else if (m_pickupTriggerMode == 2) {
                auto count = em->countForItemCustom(m_itemID);
                em->updateCountForItemReimpl(m_itemID, count / m_pickupTriggerMultiplier);
                cc.type = Divide;
                cc._val.floatVal = m_pickupTriggerMultiplier;
            } else if (!m_unkPickupBool2) {
                em->addCountToItemCustom(m_itemID, m_pickupCount);
                cc.type = Add;
                cc._val.intVal = m_pickupCount;
            } else {
                em->updateCountForItemReimpl(m_itemID, m_pickupCount);
                cc.type = Set;
                cc._val.intVal = m_pickupCount;
            }

            gjbgl->queueCounterChange(cc);

            layer->updateCounters(m_itemID, em->countForItemCustom(m_itemID));
        } else {
            CountTriggerGameObject::triggerObject(layer, idk, idunno);
        }
    }
};
