#pragma once
#include <defs/geode.hpp>

#include <Geode/modify/EditorPauseLayer.hpp>

// Defines globed-related EditorPauseLayer hooks
class $modify(GlobedEditorPauseLayer, EditorPauseLayer) {
    // callbacks

    $override
    void onSaveAndPlay(cocos2d::CCObject*);
};
