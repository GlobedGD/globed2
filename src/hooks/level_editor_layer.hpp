#pragma once
#include <defs/geode.hpp>

#include <Geode/modify/LevelEditorLayer.hpp>

class $modify(GlobedLevelEditorLayer, LevelEditorLayer) {
    static inline bool fromEditor = false;

    // gd hooks
    $override
    bool init(GJGameLevel* level, bool p1);
};
