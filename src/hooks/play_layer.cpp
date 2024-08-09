#include "play_layer.hpp"

#include "gjbasegamelayer.hpp"
#include "level_editor_layer.hpp"
#include <game/module/all.hpp>
#include <util/lowlevel.hpp>
#include <util/cocos.hpp>
#include <util/gd.hpp>

using namespace geode::prelude;

// post an event to all modules
#define GLOBED_EVENT(self, code) \
    for (auto& module : self->m_fields->modules) { \
        module->code; \
    }

// post an event to all modules
#define GLOBED_EVENT_O(self, code) \
    for (auto& module : self->m_fields->modules) { \
        auto _mo = module->code; \
        if (_mo == BaseGameplayModule::EventOutcome::Halt) return; \
    }

/* Hooks */

bool GlobedPlayLayer::init(GJGameLevel* level, bool p1, bool p2) {
    GlobedLevelEditorLayer::fromEditor = false;

    auto gjbgl = static_cast<GlobedGJBGL*>(static_cast<GJBaseGameLayer*>(this));

    gjbgl->setupPreInit(level, false);

    if (!PlayLayer::init(level, p1, p2)) return false;

    gjbgl->setupAll();

    return true;
}

void GlobedPlayLayer::setupHasCompleted() {
    // loadDeathEffect is inlined on win64, we want to avoid unloading anything
    auto* gm = GameManager::get();
    int lastLoadedEffect = gm->m_loadedDeathEffect;
    int lastDeathEffect = gm->m_playerDeathEffect;

    gm->m_loadedDeathEffect = 0;
    gm->m_playerDeathEffect = 0;

    PlayLayer::setupHasCompleted();

    gm->m_loadedDeathEffect = lastLoadedEffect;
    gm->m_playerDeathEffect = lastDeathEffect;

    util::cocos::tryLoadDeathEffect(lastDeathEffect);

    auto gjbgl = static_cast<GlobedGJBGL*>(static_cast<GJBaseGameLayer*>(this));
    gjbgl->m_fields->setupWasCompleted = true;
}

void GlobedPlayLayer::onQuit() {
    GlobedGJBGL::get()->onQuitActions();

    PlayLayer::onQuit();
}

void GlobedPlayLayer::fullReset() {
    PlayLayer::fullReset();

    auto gjbgl = GlobedGJBGL::get();

    GLOBED_EVENT_O(gjbgl, fullResetLevel());

    // turn off safe mode
    GlobedGJBGL::get()->toggleSafeMode(false);
}

void GlobedPlayLayer::resetLevel() {
    auto gjbgl = GlobedGJBGL::get();

    GLOBED_EVENT_O(gjbgl, resetLevel());

    if (m_fields->insideDestroyPlayer) {
        m_fields->insideDestroyPlayer = false;
    }

    bool lastTestMode = m_isTestMode;

    if (GlobedGJBGL::get()->isSafeMode()) {
        m_isTestMode = true;
    }

    PlayLayer::resetLevel();

    m_isTestMode = lastTestMode;

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

void GlobedPlayLayer::destroyPlayer(PlayerObject* player, GameObject* object) {
    if (!m_fields->antiCheat) {
        m_fields->antiCheat = object;
    }

    auto* pl = GlobedGJBGL::get();

    GLOBED_EVENT_O(pl, destroyPlayerPre(player, object));

    // safe mode stuff yeah

    bool lastTestMode = m_isTestMode;

    if (GlobedGJBGL::get()->isSafeMode()) {
        m_isTestMode = true;
    }

    m_fields->insideDestroyPlayer = true;

#ifdef GEODE_IS_ARM_MAC
# if GEODE_COMP_GD_VERSION != 22060
#  error "update this patch for new gd"
# else
    static auto armpatch = [] {
        return util::lowlevel::nop(0xa73e4, 0x4);
    }();

    if (armpatch) {
        if (GlobedGJBGL::get()->m_fields->shouldStopProgress) {
            (void) armpatch->enable();
        } else {
            (void) armpatch->disable();
        }
    }

    PlayLayer::destroyPlayer(player, object);

    if (armpatch && armpatch->isEnabled()) {
        (void) armpatch->disable();
    }
# endif
#else
    PlayLayer::destroyPlayer(player, object);
#endif


    GLOBED_EVENT(pl, destroyPlayerPost(player, object));

    m_isTestMode = lastTestMode;
}

void GlobedPlayLayer::forceKill(PlayerObject* p) {
    m_fields->ignoreNoclip = true;
    this->PlayLayer::destroyPlayer(p, nullptr);
    m_fields->ignoreNoclip = false;
}
