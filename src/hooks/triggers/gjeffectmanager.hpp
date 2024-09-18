#include <Geode/modify/GJEffectManager.hpp>
#include <Geode/modify/EffectGameObject.hpp>
#include <Geode/modify/CountTriggerGameObject.hpp>
#include <hooks/gjbasegamelayer.hpp>
#include <defs/platform.hpp>
#include <defs/minimal_geode.hpp>
#include <globed/constants.hpp>

struct GLOBED_DLL GJEffectManagerHook : geode::Modify<GJEffectManagerHook, GJEffectManager> {
    struct Fields {
        std::map<int, int> customItems;
    };

    static void onModify(auto& self) {
        (void) self.setHookPriority("GJEffectManager::countForItem", 999999);
    }

    $override
    void updateCountForItem(int id, int value);

    // TODO: alk fixed, wait for geode update
#ifndef GEODE_IS_WINDOWS
    $override
    int countForItem(int item);
#endif

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
