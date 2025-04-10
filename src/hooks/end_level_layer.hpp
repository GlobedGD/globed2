#pragma once
#include <defs/geode.hpp>

#ifndef GLOBED_LESS_BINDINGS

#include <Geode/modify/EndLevelLayer.hpp>

struct GLOBED_DLL HookedEndLevelLayer : geode::Modify<HookedEndLevelLayer, EndLevelLayer> {
    $override
    void customSetup();
};

#endif // GLOBED_LESS_BINDINGS
