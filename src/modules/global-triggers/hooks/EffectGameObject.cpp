#include "EffectGameObject.hpp"
#include "../CounterChange.hpp"
#include "../Ids.hpp"

#include <sinaps.hpp>

using namespace geode::prelude;

namespace globed {

void HookedEffectGameObject::triggerObject(GJBaseGameLayer* layer, int idk, gd::vector<int> const* idunno) {
    if (m_collectibleIsPickupItem && globed::isWritableCustomItem(m_itemID)) {
        CounterChange change{(uint32_t) m_itemID, m_subtractCount ? -1 : 1, CounterChangeType::Add};
        GlobalTriggersModule::get().queueCounterChange(change);
    } else if (!globed::isReadonlyCustomItem(m_itemID)) {
        EffectGameObject::triggerObject(layer, idk, idunno);
    }
}

// EffectGameObject::getSaveString patch cmp 9999 to INT_MAX to avoid checks on saving the level (item edit trigger would break otherwise)
$on_mod(Loaded) {
#if GEODE_COMP_GD_VERSION != 22074
# error "EffectGameObject::getSaveString patch requires update"
#endif

#ifndef GEODE_IS_WINDOWS
# error "EffectGameObject::getSaveString patch unimplemented for this platform"
#else

    auto doPatch = +[](uint8_t* offset) -> void {
        auto patch = Mod::get()->patch(offset, {0x3d, 0xff, 0xff, 0xff, 0x7f});
        if (!patch) {
            log::error("Failed to apply patch for EffectGameObject::getSaveString at offset {}: {}", fmt::ptr(offset - geode::base::get()), patch.unwrapErr());
        } else {
            GlobalTriggersModule::get().claimPatch(patch.unwrap());
        }
    };

    auto funcStart = reinterpret_cast<uint8_t*>(geode::base::get() + 0x4932b0);
    auto offset1 = sinaps::find<"3d 0f 27 00 00">(funcStart, 0x100);

    if (offset1 != -1) {
        doPatch(funcStart + offset1);

        auto offset2 = sinaps::find<"3d 0f 27 00 00">(funcStart + offset1 + 1, 0x100);
        if (offset2 != -1) {
            doPatch(funcStart + offset1 + offset2 + 1);
        } else {
            log::error("Failed to find second patch offset for EffectGameObject::getSaveString");
        }
    } else {
        log::error("Failed to find first patch offset for EffectGameObject::getSaveString");
    }

#endif
}

}
