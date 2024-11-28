#include "gjeffectmanager.hpp"

#ifdef GLOBED_GP_CHANGES

#include <Geode/modify/GJEffectManager.hpp>
#include <Geode/modify/EffectGameObject.hpp>
#include <Geode/modify/CountTriggerGameObject.hpp>

#include <defs/platform.hpp>
#include <defs/minimal_geode.hpp>
#include <hooks/gjbasegamelayer.hpp>
#include <globed/constants.hpp>
#include <util/lowlevel.hpp>
#include <util/sigscan.hpp>

static bool dontUpdateCountTriggers = false;

void GJEffectManagerHook::updateCountForItem(int id, int value) {
    if (globed::isWritableCustomItem(id)) {
        this->updateCountForItemCustom(id, value);
    } else {
        // for testing!
        // if (m_countTriggerActions.contains(id)) {
        //     auto& actions = m_countTriggerActions.at(id);

        //     for (const auto& action : actions) {
        //         log::debug("Action: 0 = {}, 4 = {}, count = {}, group = {}, 10 = {}, 14 = {}, 18 = {}, item id = {}, multiac = {}, set = {}", action.m_unk0, action.m_unk4, action.m_targetCount, action.m_targetGroup, action.m_unk10, action.m_unk14, action.m_unk18, action.m_itemID, action.m_multiActivate, action.m_unkVecInt);
        //         // m_triggerEffectDelegate->toggleGroupTriggered(id)
        //     }
        // }

        GJEffectManager::updateCountForItem(id, value);
    }
}

static int countForItemDetour(GJEffectManagerHook* self, int itemId) {
    if (globed::isCustomItem(itemId)) {
        return self->countForItemCustom(itemId);
    } else {
        itemId = std::clamp(itemId, 0, 9999);
        return self->m_itemIDs[itemId];
    }
}

int GJEffectManagerHook::countForItem(int item) {
    return countForItemDetour(this, item);
}

void GJEffectManagerHook::addCountToItemCustom(int id, int diff) {
    int newValue = m_fields->customItems[id] + diff;
    this->updateCountForItemCustom(id, newValue);
}

void GJEffectManagerHook::reset() {
    GJEffectManager::reset();

    m_fields->customItems.clear();
}


int GJEffectManagerHook::countForItemCustom(int id) {
    // check if it's a special read only item
    if (globed::isReadonlyCustomItem(id)) {
        return this->countForReadonlyItem(id);
    }

    return m_fields->customItems[id];
}

int GJEffectManagerHook::countForReadonlyItem(int id) {
    auto bgl = GlobedGJBGL::get();
    if (bgl && bgl->established()) {
        return bgl->countForCustomItem(id);
    }

    return 0;
}

void GJEffectManagerHook::applyFromCounterChange(const GlobedCounterChange& change) {
    using enum GlobedCounterChange::Type;

    int prev = this->countForItemCustom(change.itemId);

    switch (change.type) {
        case Add: {
            this->updateCountForItemCustom(change.itemId, prev + change._val.intVal);
        } break;
        case Set: {
            this->updateCountForItemCustom(change.itemId, change._val.intVal);
        } break;
        case Multiply: {
            this->updateCountForItemCustom(change.itemId, prev * change._val.floatVal);
        } break;
        case Divide: {
            this->updateCountForItemCustom(change.itemId, prev / change._val.floatVal);
        } break;
    }

    this->updateCountersForCustomItem(change.itemId);
}

void GJEffectManagerHook::updateCountersForCustomItem(int id) {
    GlobedGJBGL::get()->updateCounters(id, this->countForItemCustom(id));
}

void GJEffectManagerHook::applyItem(int id, int value) {
    if (this->updateCountForItemCustom(id, value)) {
        GlobedGJBGL::get()->updateCounters(id, value);
    }
}

// gjbgl collectedObject and addCountToItem inlined on windows.

// TODO: idk if this hook is needed? maybe 2.1 levels?
struct GLOBED_DLL EffectGameObjectHook : geode::Modify<EffectGameObjectHook, EffectGameObject> {
    static void onModify(auto& self) {
        GLOBED_MANAGE_HOOK(Gameplay, EffectGameObject::triggerObject);
    }

    void triggerObject(GJBaseGameLayer* layer, int idk, gd::vector<int> const* idunno) {
        if (this->m_collectibleIsPickupItem && globed::isWritableCustomItem(m_itemID)) {
            auto gjbgl = GlobedGJBGL::get();

            GlobedCounterChange cc;
            cc.itemId = globed::itemIdToCustom(m_itemID);

            using enum GlobedCounterChange::Type;

            if (m_subtractCount) {
                cc.type = Add;
                cc._val.intVal = -1;
            } else {
                cc.type = Add;
                cc._val.intVal = 1;
            }

            gjbgl->queueCounterChange(cc);
        } else if (!globed::isReadonlyCustomItem(m_itemID)){
            EffectGameObject::triggerObject(layer, idk, idunno);
        }
    }
};

static Patch* egoPatch1 = nullptr;
static Patch* egoPatch2 = nullptr;

// EffectGameObject::getSaveString patch cmp 9999 to INT_MAX to avoid checks on saving the level (item edit trigger would break otherwise)
#if GEODE_COMP_GD_VERSION == 22074
$execute {
    uintptr_t offset1 = -1, offset2 = -1, funcStart = 0;
    std::vector<uint8_t> bytes;

# pragma message "EffectGameObject::getSaveString not impl'd for this platform"
    return;

    if (offset1 == -1) {
        log::warn("Failed to find getSaveString offset 1 (searched from {:X})", funcStart);
    } else {
        egoPatch1 = util::lowlevel::patchAbsolute(offset1, bytes);
    }

    if (offset2 == -1) {
        log::warn("Failed to find getSaveString offset 2 (searched from {})", funcStart);
    } else {
        egoPatch2 = util::lowlevel::patchAbsolute(offset2, bytes);
    }
};
#else
# pragma message "EffectGameObject::getSaveString patch needs update"
#endif

void globed::toggleTriggerHooks(bool state) {
    auto toggle = [state](Patch* patch) {
        if (patch) (void) (state ? patch->enable() : patch->disable());
    };

}

void globed::toggleEditorTriggerHooks(bool state) {
    auto toggle = [state](Patch* patch) {
        if (patch) (void) (state ? patch->enable() : patch->disable());
    };

    toggle(egoPatch1);
    toggle(egoPatch2);
}

#endif // GLOBED_GP_CHANGES
