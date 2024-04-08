#include "play_layer.hpp"

#include "gjbasegamelayer.hpp"
#include "level_editor_layer.hpp"

using namespace geode::prelude;

/* Hooks */

bool GlobedPlayLayer::init(GJGameLevel* level, bool p1, bool p2) {
    if (GlobedLevelEditorLayer::fromEditor) return PlayLayer::init(level, p1, p2);

    auto gjbgl = static_cast<GlobedGJBGL*>(static_cast<GJBaseGameLayer*>(this));

    gjbgl->setupPreInit(level);

    if (!PlayLayer::init(level, p1, p2)) return false;

    gjbgl->setupAll();

    return true;
}

void GlobedPlayLayer::setupHasCompleted() {
    PlayLayer::setupHasCompleted();
    auto gjbgl = static_cast<GlobedGJBGL*>(static_cast<GJBaseGameLayer*>(this));
    gjbgl->m_fields->setupWasCompleted = true;
}

void GlobedPlayLayer::onQuit() {
    PlayLayer::onQuit();
    if (!GlobedLevelEditorLayer::fromEditor) GlobedGJBGL::get()->onQuitActions();
}

void GlobedPlayLayer::fullReset() {
    PlayLayer::fullReset();
    // turn off safe mode
    GlobedGJBGL::get()->toggleSafeMode(false);
}

void GlobedPlayLayer::resetLevel() {
    bool lastTestMode = m_isTestMode;

    if (GlobedGJBGL::get()->isSafeMode()) {
        m_isTestMode = true;
    }

    PlayLayer::resetLevel();

    m_isTestMode = lastTestMode;

    auto gjbgl = static_cast<GlobedGJBGL*>(static_cast<GJBaseGameLayer*>(this));

    // this is also called upon init, so bail out if we are too early
    if (!gjbgl->m_fields->setupWasCompleted) return;

    if (!m_level->isPlatformer()) {
        // turn off safe mode in non-platformer levels (as this counts as a full reset)
        GlobedGJBGL::get()->toggleSafeMode(false);
    }
}

void GlobedPlayLayer::showNewBest(bool p0, int p1, int p2, bool p3, bool p4, bool p5) {
    // doesn't actually change any progress but this stops the NEW BEST popup from showing up while cheating/jumping to a player
    if (!GlobedGJBGL::get()->isSafeMode()) PlayLayer::showNewBest(p0, p1, p2, p3, p4, p5);
}

void GlobedPlayLayer::levelComplete() {
    if (!GlobedGJBGL::get()->isSafeMode()) PlayLayer::levelComplete();
    else GlobedPlayLayer::onQuit();
}

void GlobedPlayLayer::destroyPlayer(PlayerObject* p0, GameObject* p1) {
    bool lastTestMode = m_isTestMode;

    if (GlobedGJBGL::get()->isSafeMode()) {
        m_isTestMode = true;
    }

    PlayLayer::destroyPlayer(p0, p1);

    m_isTestMode = lastTestMode;
}
