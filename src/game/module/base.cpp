#include "base.hpp"

#include <hooks/gjbasegamelayer.hpp>
#include <hooks/play_layer.hpp>
#include <hooks/level_editor_layer.hpp>

GlobedPlayLayer* BaseGameplayModule::getPlayLayer() {
    return static_cast<GlobedPlayLayer*>(typeinfo_cast<PlayLayer*>(static_cast<GJBaseGameLayer*>(gameLayer)));
}

GlobedLevelEditorLayer* BaseGameplayModule::getEditorLayer() {
    return static_cast<GlobedLevelEditorLayer*>(typeinfo_cast<LevelEditorLayer*>(static_cast<GJBaseGameLayer*>(gameLayer)));
}