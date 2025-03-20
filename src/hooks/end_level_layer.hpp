#pragma once
#include <defs/geode.hpp>

#ifndef GLOBED_DISABLE_EXTRA_HOOKS

#include <Geode/modify/EndLevelLayer.hpp>

struct GLOBED_DLL HookedEndLevelLayer : geode::Modify<HookedEndLevelLayer, EndLevelLayer> {
    $override
    void customSetup();
};

#endif // GLOBED_DISABLE_EXTRA_HOOKS
