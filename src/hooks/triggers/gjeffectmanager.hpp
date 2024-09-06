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

    void addCountToItemCustom(int id, int diff);

    void updateCountForItemReimpl(int id, int value);

    int countForItemCustom(int id);

    void applyFromCounterChange(const GlobedCounterChange& change);
};
