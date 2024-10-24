#include <defs/platform.hpp>
#include <defs/minimal_geode.hpp>

#include <hooks/gjbasegamelayer.hpp>
#include <globed/constants.hpp>
#include <managers/hook.hpp>

#include <Geode/modify/GJEffectManager.hpp>
#include <Geode/modify/EffectGameObject.hpp>
#include <Geode/modify/CountTriggerGameObject.hpp>


struct GLOBED_DLL GJEffectManagerHook : geode::Modify<GJEffectManagerHook, GJEffectManager> {
    struct Fields {
        std::map<int, int> customItems;
    };

    static void onModify(auto& self) {
        (void) self.setHookPriority("GJEffectManager::countForItem", 999999);
        GLOBED_MANAGE_HOOK(Gameplay, GJEffectManager::countForItem);
        GLOBED_MANAGE_HOOK(Gameplay, GJEffectManager::updateCountForItem);
        GLOBED_MANAGE_HOOK(Gameplay, GJEffectManager::reset);
    }

    $override
    void updateCountForItem(int id, int value);

    $override
    int countForItem(int item);

    $override
    void reset();

    void addCountToItemCustom(int id, int diff);

    bool updateCountForItemCustom(int id, int value);

    int countForItemCustom(int id);
    int countForReadonlyItem(int id);

    void updateCountersForCustomItem(int id);

    [[deprecated]] void applyFromCounterChange(const GlobedCounterChange& change);
    void applyItem(int id, int value);
};

namespace globed {
    // enable/disable trigger-related patch hooks
    void toggleTriggerHooks(bool state);

    // enable/disable editor trigger-related patch hooks
    void toggleEditorTriggerHooks(bool state);
}