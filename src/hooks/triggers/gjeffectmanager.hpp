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

    $override
    void updateCountForItem(int id, int value);

    // TODO: maybe inline flag will help here
#ifndef GEODE_IS_WINDOWS
    $override
    int countForItem(int item);
#endif

    $override
    void reset();

    void addCountToItemCustom(int id, int diff);

    void updateCountForItemCustom(int id, int value);

    int countForItemCustom(int id);

    [[deprecated]] void applyFromCounterChange(const GlobedCounterChange& change);
    void applyItem(int id, int value);
};
