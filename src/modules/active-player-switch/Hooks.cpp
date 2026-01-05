#include "Hooks.hpp"

#include <globed/core/RoomManager.hpp>
#include <globed/util/Random.hpp>
#include <core/hooks/GJBaseGameLayer.hpp>
#include <core/net/NetworkManagerImpl.hpp>

#include <UIBuilder.hpp>

using namespace geode::prelude;

static bool g_ignoreNoclip = false;
static bool g_ignoreButtonBlock = true;

// when respawning player becomes visible again

namespace globed {

bool APSPlayLayer::init(GJGameLevel* level, bool a, bool b) {
    if (!PlayLayer::init(level, a, b)) return false;

    m_fields->m_listener = NetworkManagerImpl::get().listen<msg::LevelDataMessage>([this](const msg::LevelDataMessage& msg) {
        for (auto& event : msg.events) {
            if (event.is<ActivePlayerSwitchEvent>()) {
                this->handleEvent(event.as<ActivePlayerSwitchEvent>());
            }
        }

        return ListenerResult::Continue;
    });

    float glowScale = 1.5f;

    m_fields->m_switchGlow = Build(CCScale9Sprite::create("switch-screen-glow.png"_spr))
        .scale(glowScale)
        .zOrder(11)
        .contentSize(CCDirector::get()->getWinSize() / glowScale)
        .visible(false)
        .id("switch-glow"_spr)
        .parent(m_uiLayer)
        .anchorPoint(0.f, 0.f);

    m_fields->m_switchPreglow = Build(CCScale9Sprite::create("switch-screen-preglow.png"_spr))
        .scale(glowScale)
        .zOrder(10)
        .contentSize(CCDirector::get()->getWinSize() / glowScale)
        .visible(false)
        .id("switch-preglow"_spr)
        .parent(m_uiLayer)
        .anchorPoint(0.f, 0.f);

    auto gjbgl = GlobedGJBGL::get(this);
    gjbgl->toggleCullingEnabled(false);
    gjbgl->toggleExtendedData(true);
    auto& lerper = gjbgl->m_fields->m_interpolator;
    lerper.setLowLatencyMode(true);
    lerper.setCameraCorrections(false);

    return true;
}

void APSPlayLayer::destroyPlayer(PlayerObject* player, GameObject* obj) {
    auto& mod = APSModule::get();
    auto& fields = *m_fields.self();
    auto gameLayer = GlobedGJBGL::get();

    if (!gameLayer || fields.m_meActive || fields.m_activePlayer == 0) {
        PlayLayer::destroyPlayer(player, obj);
        return;
    }

    // noclip if spectating another player
    if (obj != m_anticheatSpike && !g_ignoreNoclip && (player == gameLayer->m_player1 || player == gameLayer->m_player2)) {
        // log::debug("(APS) Noclipping death");
        return;
    }

    PlayLayer::destroyPlayer(player, obj);
}

void APSPlayLayer::handleEvent(const ActivePlayerSwitchEvent& event) {
    log::info("(APS) Switching active player to {} (type: {})", event.playerId, event.type);

    auto self = GlobedGJBGL::get(this);

    if (event.type == 2) { // full reset
        this->customResetLevel();
    }

    if (event.type == 0) { // pre-switch
        if (event.playerId == ::get<GJAccountManager>()->m_accountID) {
            this->showPreSwitchEffect();
        }

        return;
    }

    auto& fields = *m_fields.self();
    fields.m_activePlayer = event.playerId;
    fields.m_meActive = event.playerId == ::get<GJAccountManager>()->m_accountID;

    for (auto& [id, player] : self->m_fields->m_players) {
        if (id == fields.m_activePlayer) {
            player->setForceHide(false);
        } else {
            player->setForceHide(true);
        }
    }

    if (fields.m_meActive) {
        float delay = rng::generate(3.5f, 8.0f);

        auto scene = this->getParent();
        scene->scheduleOnce(schedule_selector(APSPlayLayer::performPreSwitchProxy), delay);
    } else {
        // switched away from us, cancel inputs
        bool prev = g_ignoreButtonBlock;
        g_ignoreButtonBlock = true;
        GJBaseGameLayer::handleButton(false, 0, true);
        g_ignoreButtonBlock = prev;
    }

    this->showSwitchEffect();
}

void APSPlayLayer::showSwitchEffect() {
    this->showEffect(false);
}

void APSPlayLayer::showPreSwitchEffect() {
    this->showEffect(true);
}

void APSPlayLayer::showEffect(bool presw) {
    auto& fields = *m_fields.self();
    auto node = presw ? fields.m_switchPreglow : fields.m_switchGlow;

    // first disable both
    fields.m_switchPreglow->setVisible(false);
    fields.m_switchPreglow->stopAllActions();
    fields.m_switchGlow->setVisible(false);
    fields.m_switchGlow->stopAllActions();

    node->setOpacity(0);
    node->runAction(
        CCSequence::create(
            CCShow::create(),
            CCFadeIn::create(presw ? 0.01f : 0.f),
            CCDelayTime::create(presw ? 5.f : 0.4f),
            CCFadeOut::create(0.1f),
            CCHide::create(),
            nullptr
        )
    );
}

void APSPlayLayer::handlePlayerDeath(const PlayerDeath& death, RemotePlayer* player) {
    if (player->id() == m_fields->m_activePlayer) {
        log::info("Handling death from {}", player->displayData().username);
        g_ignoreNoclip = true;
        GlobedGJBGL::get(this)->killLocalPlayer();
        g_ignoreNoclip = false;
    }
}

void APSPlayLayer::customResetLevel() {
    auto scene = CCScene::get();

    auto pauselayer = scene->getChildByType<PauseLayer>(0);
    if (!pauselayer) {
        this->pauseGame(false);
        pauselayer = scene->getChildByType<PauseLayer>(0);
    }

    this->resumeAndRestart(true);

    if (pauselayer) {
        pauselayer->removeFromParent();
    }
}

bool APSPlayLayer::shouldBlockInput() {
    auto& fields = *m_fields.self();
    return !fields.m_meActive && fields.m_activePlayer != 0;
}

void APSPlayLayer::performSwitch() {
    auto& fields = *m_fields.self();
    log::info("(APS) Sending switch event to {}", fields.m_switchingTo);

    NetworkManagerImpl::get().queueGameEvent(ActivePlayerSwitchEvent {
        .playerId = fields.m_switchingTo,
        .type = 1, // switch
    });
}

void APSPlayLayer::performPreSwitch() {
    auto& fields = *m_fields.self();
    auto self = GlobedGJBGL::get(this);

    // choose the next player
    auto& players = self->m_fields->m_players;

    if (players.empty()) {
        log::warn("no players to switch to!");
        return;
    }

    size_t idx = rng::generate<size_t>(0, players.size() - 1);

    auto it = players.begin();
    std::advance(it, idx);

    fields.m_switchingTo = it->first;
    log::info("(APS) Sending pre-switch event to {}", it->first);

    NetworkManagerImpl::get().queueGameEvent(ActivePlayerSwitchEvent {
        .playerId = fields.m_switchingTo,
        .type = 0, // pre-switch
    });

    // schedule the switch 1 second later
    auto scene = this->getParent();
    scene->scheduleOnce(schedule_selector(APSPlayLayer::performSwitchProxy), 1.0f);

    this->showPreSwitchEffect();
}

void APSPlayLayer::performSwitchProxy(float dt) {
    if (auto pl = APSPlayLayer::get()) pl->performSwitch();
}

void APSPlayLayer::performPreSwitchProxy(float dt) {
    if (auto pl = APSPlayLayer::get()) pl->performPreSwitch();
}

void APSPlayLayer::handleUpdate() {
    auto& fields = *m_fields.self();
    auto self = GlobedGJBGL::get(this);

    if (fields.m_activePlayer != 0) {
        bool p1vis = fields.m_meActive;
        bool p2vis = fields.m_meActive && m_gameState.m_isDualMode;

        m_player1->setVisible(p1vis);
        if (m_player1->m_shipStreak) m_player1->m_shipStreak->setVisible(p1vis);
        if (m_player1->m_regularTrail) m_player1->m_regularTrail->setVisible(p1vis);

        m_player2->setVisible(p2vis);
        if (m_player2->m_shipStreak) m_player2->m_shipStreak->setVisible(p2vis);
        if (m_player2->m_regularTrail) m_player2->m_regularTrail->setVisible(p2vis);
    }

    if (!fields.m_meActive && fields.m_activePlayer != 0) {
        auto rp = self->getPlayer(fields.m_activePlayer);
        if (rp) {
            this->handleUpdateFromRp(m_player1, rp.get(), false);
            this->handleUpdateFromRp(m_player2, rp.get(), true);
        } else {
            log::warn("active player {} not found!", fields.m_activePlayer);
            fields.m_activePlayer = 0;
        }
    }
}

void APSPlayLayer::handleUpdateFromRp(PlayerObject* local, RemotePlayer* rp, bool isPlayer2) {
    auto p1 = isPlayer2 ? rp->player2() : rp->player1();
    if (p1->m_isDead) return;

    auto pos = p1->getLastPosition();

    local->setPosition(pos);
    local->m_position = pos;
    local->m_yVelocity = p1->m_yVelocity;
    local->m_platformerXVelocity = p1->m_platformerXVelocity;
    local->m_isOnGround = p1->m_isOnGround;
    local->m_isGoingLeft = p1->m_isGoingLeft;
    local->m_isAccelerating = p1->m_isAccelerating;
    local->m_accelerationOrSpeed = p1->m_accelerationOrSpeed;
    local->m_fallStartY = p1->m_fallStartY;
    local->m_gravity = p1->m_gravity;
    local->m_gravityMod = p1->m_gravityMod;
    local->m_isOnGround2 = p1->m_isOnGround2;
    local->m_touchedPad = p1->m_touchedPad;
    local->m_isUpsideDown = p1->m_isUpsideDown;

    // m_player1->m_isRotating = p1->m_isRotating;
    // m_player1->m_isSideways = p1->m_isSideways;
    // m_player1->m_isDashing = p1->m_isDashing;
}

APSPlayLayer* APSPlayLayer::get(GJBaseGameLayer* gjbgl) {
    if (!gjbgl || gjbgl->m_isEditor) {
        gjbgl = ::get<GameManager>()->m_playLayer;
    }

    return static_cast<APSPlayLayer*>(gjbgl);
}

// GJBGL

void APSGJBGL::handleButton(bool a, int b, bool c) {
    auto pl = APSPlayLayer::get(this);
    if (!pl || !pl->shouldBlockInput() || g_ignoreButtonBlock) {
        GJBaseGameLayer::handleButton(a, b, c);
    }
}

void APSGJBGL::update(float dt) {
    GJBaseGameLayer::update(dt);

    auto pl = APSPlayLayer::get(this);
    if (pl) pl->handleUpdate();
}

// PauseLayer

void APSPauseLayer::customSetup() {
    PauseLayer::customSetup();

    if (!NetworkManagerImpl::get().isConnected()) {
        return;
    }

    auto& rm = RoomManager::get();
    if (!rm.isOwner()) {
        return;
    }

    auto winSize = ::get<CCDirector>()->getWinSize();

    auto menu = this->getChildByID("playerlist-menu"_spr);
    if (!menu) {
        log::error("playerlist-menu not found, cannot add aps button!");
        return;
    }

    Build<ButtonSprite>::create("Start", "bigFont.fnt", "GJ_button_01.png", 0.7f)
        .scale(0.7f)
        .intoMenuItem([] {
            NetworkManagerImpl::get().queueGameEvent(ActivePlayerSwitchEvent {
                .playerId = GJAccountManager::get()->m_accountID,
                .type = 2,
            });
        })
        .parent(menu);

    menu->updateLayout();
}

}
