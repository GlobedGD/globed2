#pragma once
#include <defs/geode.hpp>

#include <Geode/modify/EndLevelLayer.hpp>

struct GLOBED_DLL HookedEndLevelLayer : geode::Modify<HookedEndLevelLayer, EndLevelLayer> {
    $override
    void customSetup();
};