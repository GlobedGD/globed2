#pragma once

#include "../GlobalTriggersModule.hpp"
#include <Geode/Geode.hpp>
#include <Geode/modify/GJEffectManager.hpp>
#include <globed/config.hpp>

namespace globed {

struct GLOBED_MODIFY_ATTR HookedGJEffectManager : geode::Modify<HookedGJEffectManager, GJEffectManager> {
    struct Fields {
        std::unordered_map<int, int> m_customItems;
    };

    static void onModify(auto &self)
    {
        (void)self.setHookPriority("GJEffectManager::countForItem", 999999);

        GLOBED_CLAIM_HOOKS(GlobalTriggersModule::get(), self, "GJEffectManager::countForItem",
                           "GJEffectManager::updateCountForItem", "GJEffectManager::reset", );
    }

    $override int countForItem(int itemId);

    // Contract: this function must never be called for a non-custom item. Use `countForItem` if uncertain.
    int countForCustomItem(int itemId);

    $override void updateCountForItem(int itemId, int value);

    // Contract: same as above
    bool updateCountForCustomItem(int itemId, int value);

    $override void reset();
};

} // namespace globed
