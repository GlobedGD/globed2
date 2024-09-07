#include "gjeffectmanager.hpp"

#include <Geode/modify/GJEffectManager.hpp>
#include <Geode/modify/EffectGameObject.hpp>
#include <Geode/modify/CountTriggerGameObject.hpp>

#include <defs/platform.hpp>
#include <defs/minimal_geode.hpp>
#include <hooks/gjbasegamelayer.hpp>
#include <globed/constants.hpp>
#include <util/lowlevel.hpp>

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

#ifndef GEODE_IS_WINDOWS
int GJEffectManagerHook::countForItem(int item) {
    if (inRange(item)) {
        return this->countForItemCustom(item);
    } else {
        return GJEffectManager::countForItem(item);
    }
}
#else

// We can't hook this function because gd's LTO assumes it will not modify r11,
// which our hook will inadvertently do, breaking one or multiple callsites.

// The patch preserves r11 and calls a reimplementation of the function.

static geode::Patch* countForItemPatch = nullptr;

int countForItemDetour(GJEffectManagerHook* self, int itemId) {
    if (inRange(itemId)) {
        return self->countForItemCustom(itemId);
    } else {
        itemId = std::clamp(itemId, 0, 9999);
        return self->m_itemIDs[itemId];
    }
}

$execute {
    std::vector<uint8_t> bytes = {
        0x41, 0x53, // push r11
        0x48, 0x83, 0xec, 0x10, // sub rsp, 0x10
        0x48, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // movabs rax, 0x0
        0xff, 0xd0, // call rax
        0x48, 0x83, 0xc4, 0x10, // add rsp, 0x10
        0x41, 0x5b, // pop r11
        0xc3, // ret
    };

    uint64_t daddr = reinterpret_cast<uint64_t>(&countForItemDetour);
    std::memcpy(bytes.data() + 8, &daddr, sizeof(daddr));

    // for some reason getNonVirtual does not work here lol
#if GEODE_COMP_GD_VERSION == 22060
countForItemPatch = util::lowlevel::patch(0x2506d0, bytes);
#else
# error Update this for new gd, is address of GJEffectManager::countForItem
#endif
}
#endif

void GJEffectManagerHook::addCountToItemCustom(int id, int diff) {
    int newValue = m_fields->customItems[id] + diff;
    this->updateCountForItemReimpl(id, newValue);
}

void GJEffectManagerHook::reset() {
    GJEffectManager::reset();

    m_fields->customItems.clear();
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
