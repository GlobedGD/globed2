#include "GJBaseGameLayer.hpp"
#include <globed/util/singleton.hpp>
#include <globed/util/gd.hpp>
#include <globed/core/RoomManager.hpp>
#include <core/CoreImpl.hpp>
#include <core/PreloadManager.hpp>

#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/CurrencyRewardLayer.hpp>
#include <Geode/cocos/support/data_support/uthash.h>

using namespace geode::prelude;

static bool g_speedingUpRewards = false;
static float g_speedUpFactor = 0.f;
// static asp::time::Instant g_diedAt = asp::time::Instant::now();

namespace cocos2d {
typedef struct _hashElement {
    struct _ccArray             *actions;
    CCObject                    *target;
    unsigned int                actionIndex;
    CCAction                    *currentAction;
    bool                        currentActionSalvaged;
    bool                        paused;
    UT_hash_handle                hh;
} tHashElement;
}

namespace globed {

static std::vector<Ref<CCAction>> getAllActions(CCNode* target) {
    std::vector<Ref<CCAction>> out;

    auto am = target->m_pActionManager;

    tHashElement *pElement = NULL;
    HASH_FIND_INT(am->m_pTargets, &target, pElement);

    if (pElement) {
        if (pElement->actions != NULL) {
            unsigned int limit = pElement->actions->num;
            for (unsigned int i = 0; i < limit; ++i) {
                out.push_back((CCAction*)pElement->actions->arr[i]);
            }
        }
    }

    return out;
}

static void _decomposeRecurse(CCSequence* seq, std::vector<CCAction*>& out) {
    auto first = seq->m_pActions[0];
    auto second = seq->m_pActions[1];

    if (auto fseq = typeinfo_cast<CCSequence*>(first)) {
        _decomposeRecurse(fseq, out);
    } else {
        out.push_back(first);
    }

    if (auto sseq = typeinfo_cast<CCSequence*>(second)) {
        _decomposeRecurse(sseq, out);
    } else {
        out.push_back(second);
    }
}

static std::vector<CCAction*> decomposeSequence(CCSequence* seq) {
    std::vector<CCAction*> actions;
    _decomposeRecurse(seq, actions);

    return actions;
}

static void _durRecurse(CCSequence* seq) {
    auto a = seq->m_pActions[0];
    auto b = seq->m_pActions[1];

    if (auto aseq = typeinfo_cast<CCSequence*>(a)) {
        _durRecurse(aseq);
    }

    if (auto bseq = typeinfo_cast<CCSequence*>(b)) {
        _durRecurse(bseq);
    }

    float d = a->getDuration() + b->getDuration();
    seq->initWithDuration(d);

    seq->m_split = a->getDuration() / seq->m_fDuration;
    seq->m_last = -1;
}

static void recalculateSequenceSplit(CCSequence* seq) {
    _durRecurse(seq);
}

struct GLOBED_MODIFY_ATTR HookedPlayLayer : geode::Modify<HookedPlayLayer, PlayLayer> {
    struct Fields {
        bool m_setupWasCompleted = false;
        bool m_showedNewBest = false;
    };

    GlobedGJBGL* asBase() {
        return GlobedGJBGL::get(this);
    }

    static void onModify(auto& self) {
        (void) self.setHookPriority("PlayLayer::destroyPlayer", 0x500000);
    }

    $override
    bool init(GJGameLevel* level, bool a, bool b) {
        auto gjbgl = this->asBase();
        gjbgl->setupPreInit(level, false);

        if (!PlayLayer::init(level, a, b)) return false;

        gjbgl->setupPostInit();

        return true;
    }

    $override
    void onQuit() {
        this->asBase()->onQuit();
        PlayLayer::onQuit();
    }

    $override
    void setupHasCompleted() {
        // This function calls `GameManager::loadDeathEffect` (inlined on windows),
        // which unloads the current death effect (this interferes with our preloading mechanism).
        // Override it and temporarily set death effect to 0, to prevent it from being unloaded.

        auto& pm = PreloadManager::get();
        if (!pm.deathEffectsLoaded()) {
            // if they were not preloaded, skip custom behavior and let the game handle it normally
            PlayLayer::setupHasCompleted();
            return;
        }

        auto gm = globed::cachedSingleton<GameManager>();
        int effect = gm->m_playerDeathEffect;

        gm->m_loadedDeathEffect = 0;
        gm->m_playerDeathEffect = 0;

        PlayLayer::setupHasCompleted();

        // TODO: the progress bar indicators

        gm->m_playerDeathEffect = effect;
        gm->m_loadedDeathEffect = effect;

        m_fields->m_setupWasCompleted = true;
    }

    $override
    void destroyPlayer(PlayerObject* player, GameObject* obj) {
        auto& rm = RoomManager::get();
        bool overrideReset = !rm.isInGlobal() && this->asBase()->active();
        bool oldReset = overrideReset ? gameVariable(GameVariable::FastRespawn) : false;
        // bool speedUpAnim = CoreImpl::get().shouldSpeedUpNewBest(GlobedGJBGL::get(this));
        bool speedUpAnim = true;
        bool realDeath = m_fields->m_setupWasCompleted && obj != m_anticheatSpike;

        if (overrideReset) {
            setGameVariable(GameVariable::FastRespawn, rm.getSettings().fasterReset);
        }

        // g_diedAt = asp::time::Instant::now();
        PlayLayer::destroyPlayer(player, obj);

        if (overrideReset) {
            setGameVariable(GameVariable::FastRespawn, oldReset);
        }

        if (realDeath) {
            // if this showed a new best popup, it can mess up players in 2 player mode or deathlink
            if (speedUpAnim) {
                this->speedUpNewBest(overrideReset ? rm.getSettings().fasterReset : gameVariable(GameVariable::FastRespawn));
            }

            GlobedGJBGL::get(this)->handleLocalPlayerDeath(player);
        }
    }

    $override
    void resetLevel() {
        PlayLayer::resetLevel();

        if (!m_fields->m_setupWasCompleted) return;

        // log::debug("Respawned after {}", g_diedAt.elapsed().toString());

        auto gjbgl = GlobedGJBGL::get(this);
        if (!m_level->isPlatformer()) {
            // turn off safe mode in non-platformer levels (as this counts as a full reset)
            gjbgl->resetSafeMode();
        }
    }

    $override
    void fullReset() {
        PlayLayer::fullReset();

        auto gjbgl = GlobedGJBGL::get(this);
        gjbgl->resetSafeMode();
    }

    $override
    void showNewBest(bool p0, int p1, int p2, bool p3, bool p4, bool p5) {
        // doesn't actually change any progress but this stops the NEW BEST popup from showing up while cheating/jumping to a player
        if (!GlobedGJBGL::get(this)->isSafeMode()) {
            PlayLayer::showNewBest(p0, p1, p2, p3, p4, p5);
            m_fields->m_showedNewBest = true;
        }
    }

    void speedUpNewBest(bool fastRespawn) {
        // No new best popup:
        // 1 CCDelayTime (0.5 / 1.0) -> CCCallFunc (delayedResetLevel)

        // New best popup with no diamonds/orbs
        // 1 CCDelayTime (1.4) -> CCCallFunc (delayedResetLevel?)
        // 2 CCEaseElasticOut(CCScaleTo(0.4)) -> CCDelayTime (0.7) -> CCEaseIn(CCScaleTo(0.2)) -> CCHide -> CCCallFunc (removeMeAndCleanup?)

        // Diamonds / orbs
        // 2 CCEaseElasticOut(CCScaleTo(0.4)) -> CCDelayTime (1.3) -> CCEaseIn(CCScaleTo(0.2)) -> CCHide -> CCCallFunc (removeMeAndCleanup?)
        // 3 CCFadeTo(0.3) -> CCDelayTime(1.6) -> CCFadeTo(0.4) -> CCCallFunc (removeMeAndCleanup?)

        // actions 1 are ran on playlayer
        // actions 2 are ran on a ccnode created in playlayer
        // actions 3 are ran on a cclayercolor created in playlayer
        // additionally there's a 4th thing, CurrencyRewardLayer::update

        // this means, to get the duration to be in line, we have to, in specific cases:
        // x. set t = 0.5 or 1.0 depending on the faster reset setting
        // 1. set CCDelayTime delay to t
        // 2. (for t = 0.5) change both CCScaleTo to 0.1, change CCDelayTime to 0.3
        // 2. (for t = 1.0) change first CCScaleTo to 0.2, change CCDelayTime to 0.6
        // 3. (for t = 0.5) change both fades to 0.1 and delay to 0.3
        // 3. (for t = 1.0) change delay to 0.3
        // 4.
        float t = fastRespawn ? 0.5f : 1.0f;

        auto doCase1 = [&](CCSequence* seq, std::vector<CCAction*> actions) {
            auto dt = static_cast<CCDelayTime*>(actions[0]);
            log::debug("Running case 1 for new best speedup, {} -> {}", dt->m_fDuration, t);

            dt->m_fDuration = t;
            recalculateSequenceSplit(seq);
        };

        auto doCase2 = [&](CCSequence* seq, std::vector<CCAction*> actions) {
            log::debug("Running case 2 for new best speedup (t = {})", t);

            auto firstEase = static_cast<CCEaseElasticOut*>(actions[0]);
            auto delay = static_cast<CCDelayTime*>(actions[1]);
            auto secondEase = static_cast<CCEaseElasticOut*>(actions[2]);

            if (fastRespawn) {
                firstEase->m_fDuration = 0.1f;
                firstEase->m_pInner->m_fDuration = 0.1f;
                delay->m_fDuration = 0.3f;
                secondEase->m_fDuration = 0.1f;
                secondEase->m_pInner->m_fDuration = 0.1f;
            } else {
                firstEase->m_fDuration = 0.2f;
                firstEase->m_pInner->m_fDuration = 0.2f;
                delay->m_fDuration = 0.5f;
            }

            recalculateSequenceSplit(seq);
        };

        auto doCase3 = [&](CCSequence* seq, std::vector<CCAction*> actions) {
            g_speedingUpRewards = true;
            g_speedUpFactor = fastRespawn ? 3.2f : 1.6f;

            log::debug("Running case 3 for new best speedup (t = {})", t);

            auto fade1 = static_cast<CCFadeTo*>(actions[0]);
            auto delay = static_cast<CCDelayTime*>(actions[1]);
            auto fade2 = static_cast<CCFadeTo*>(actions[2]);

            if (fastRespawn) {
                fade1->m_fDuration = 0.1f;
                delay->m_fDuration = 0.3f;
                fade2->m_fDuration = 0.1f;
            } else {
                delay->m_fDuration = 0.3f;
            }

            recalculateSequenceSplit(seq);
        };

        for (auto action : getAllActions(this)) {
            auto seq = typeinfo_cast<CCSequence*>(*action);
            if (!seq) return;

            // decompose the sequence
            auto actions = decomposeSequence(seq);

            // check if this really is our case 1
            if (actions.size() == 2 && typeinfo_cast<CCDelayTime*>(actions[0]) && typeinfo_cast<CCCallFunc*>(actions[1])) {
                doCase1(seq, actions);
            }
        }

        // find the nodes for cases 2 and 3
        for (auto child : this->getChildrenExt()) {
            if (!child->getID().empty()) continue;

            if (child->getZOrder() == 100) {
                auto sprite = geode::cocos::getChildBySpriteFrameName(child, "GJ_newBest_001.png");
                if (!sprite) continue;

                // we found it!
                auto actions = getAllActions(child);
                for (auto action : actions) {
                    auto seq = typeinfo_cast<CCSequence*>(*action);
                    if (!seq) continue;

                    auto actions = decomposeSequence(seq);
                    if (actions.size() != 5) continue;

                    if (
                        typeinfo_cast<CCEaseElasticOut*>(actions[0])
                        && typeinfo_cast<CCDelayTime*>(actions[1])
                        && typeinfo_cast<CCEaseIn*>(actions[2])
                        && typeinfo_cast<CCHide*>(actions[3])
                        && typeinfo_cast<CCCallFunc*>(actions[4])
                    ) {
                        doCase2(seq, actions);
                    }
                }
            } else if (child->getZOrder() == 99) {
                auto cclc = typeinfo_cast<CCLayerColor*>(child);
                if (!cclc) continue;

                if (cclc->getColor() == ccColor3B{0, 0, 0}) {
                    auto actions = getAllActions(child);
                    for (auto action : actions) {
                        auto seq = typeinfo_cast<CCSequence*>(*action);
                        if (!seq) continue;

                        auto actions = decomposeSequence(seq);
                        if (actions.size() != 4) continue;

                        if (
                            typeinfo_cast<CCFadeTo*>(actions[0])
                            && typeinfo_cast<CCDelayTime*>(actions[1])
                            && typeinfo_cast<CCFadeTo*>(actions[2])
                            && typeinfo_cast<CCCallFunc*>(actions[3])
                        ) {
                            doCase3(seq, actions);
                        }
                    }
                }
            }
        }
    }
};

}

class $modify(CurrencyRewardLayer) {
    void update(float dt) {
        if (g_speedingUpRewards) {
            dt *= g_speedUpFactor;
        }

        CurrencyRewardLayer::update(dt);

        if (m_objects->count() == 0) {
            g_speedingUpRewards = false;
        }
    }
};
