#include "level_editor_layer.hpp"

#include "gjbasegamelayer.hpp"

using namespace geode::prelude;

/* Hooks */

bool GlobedLevelEditorLayer::init(GJGameLevel* level, bool p1) {
    GlobedLevelEditorLayer::fromEditor = true;

    auto gjbgl = static_cast<GlobedGJBGL*>(static_cast<GJBaseGameLayer*>(this));

    gjbgl->setupPreInit(level, true);

    if (!LevelEditorLayer::init(level, p1)) return false;

    gjbgl->setupAll();

    gjbgl->m_fields->setupWasCompleted = true;

    return true;
}
