#include "Hooks.hpp"

#include "SettingsPopup.hpp"
#include <globed/core/RoomManager.hpp>
#include <globed/core/data/Event.hpp>
#include <globed/util/gd.hpp>
#include <globed/util/mh.hpp>
#include <Geode/utils/random.hpp>
#include <core/hooks/GJBaseGameLayer.hpp>
#include <core/net/NetworkManagerImpl.hpp>

#include <UIBuilder.hpp>
#include <asp/iter.hpp>

using namespace geode::prelude;

static bool g_ignoreNoclip = false;
static bool g_ignoreButtonBlock = true;
static bool g_p1Held = false;
static bool g_p2Held = false;

static bool g_realP1Held = false;
static bool g_realP2Held = false;

// when respawning player becomes visible again

namespace globed {

using SwitchType = SwitcherooSwitchEvent::Type;

static void buttonPress(GlobedGJBGL* gjbgl, bool held, bool p2) {
    auto& b = p2 ? g_p2Held : g_p1Held;
    if (held == b) return;

    g_ignoreButtonBlock = true;
    gjbgl->handleButton(held, 1, !p2);
    g_ignoreButtonBlock = false;
}

// Game controller

void APSController::restart() {
    m_activePlayer = m_pl->m_fields->m_myAccountId;
    m_meActive = true;
    m_gameActive = true;
    this->calculateNextSwitch();
}

void APSController::handleStateEvent(const SwitcherooFullStateEvent& event) {
    // is the settings popup currently open? if so, delay this till next frame to fix a nasty touch prio bug
    if (CCScene::get()->getChildByType<APSSettingsPopup>(0)) {
        FunctionQueue::get().queue([sr = WeakRef(m_pl), event] {
            auto pl = sr.lock();
            if (!pl) return;
            pl->m_fields->m_controller.handleStateEvent(event);
        });
        return;
    }

    log::debug(
        "(APS) State: game {}, player {}, indication {}, restarting {}",
        event.gameActive, event.activePlayer, event.playerIndication, event.restarting
    );

    m_settings.m_showNextPlayer = event.playerIndication;
    m_gameActive = event.gameActive;
    m_activePlayer = event.activePlayer;
    m_meActive = event.activePlayer == m_pl->m_fields->m_myAccountId;

    if (event.restarting) {
        m_gjbgl->causeLocalRespawn();
    }

    this->rehidePlayers();
}

void APSController::handleSwitchEvent(const SwitcherooSwitchEvent& event) {
    if (event.type == event.Warning) {
        log::debug("(APS) Soon switching to {}", event.playerId);
        m_nextPlayer = event.playerId;

        // if showNextPlayer is enabled, only show the pre switch effect to the next player, to make it simpler
        // otherwise show to everyone and let them guess!
        bool meNext = m_nextPlayer == m_pl->m_fields->m_myAccountId;

        if (m_settings.m_showNextPlayer) {
            if (meNext) m_pl->showPreSwitchEffect();
            m_pl->showNextPlayerNotification(m_nextPlayer);
        } else {
            m_pl->showPreSwitchEffect();
        }

    } else if (event.type == event.Switch) {
        log::debug("(APS) Now switching to {}!", event.playerId);
        m_activePlayer = event.playerId;
        m_meActive = m_activePlayer == m_pl->m_fields->m_myAccountId;
        m_pl->hideSwitchEffects();

        if (m_meActive) {
            m_pl->showSwitchEffect();

            // did it switch to us while dead? if so, respawn because otherwise we'll be dead forever
            if (m_pl->m_player1->m_isDead) {
                log::debug("(APS) Switched to us while dead, respawning...");
                m_gjbgl->causeLocalRespawn();
            }
        }

        this->rehidePlayers();
        this->repushButtons();
    }
}

void APSController::rehidePlayers() {
    for (auto& [id, player] : m_gjbgl->m_fields->m_players) {
        player->setForceHide(id != m_activePlayer);
    }
    m_gjbgl->setSpectating(!m_meActive);
}

void APSController::repushButtons() {
    buttonPress(m_gjbgl, g_realP1Held, false);
    buttonPress(m_gjbgl, g_realP2Held, true);
}

void APSController::rescheduleNextSwitch(float delayS) {
    m_nextSwitch = asp::Instant::now() + *asp::Duration::fromSecs(delayS);
}

std::optional<SwitcherooSwitchEvent> APSController::poll() {
    if (!m_gameActive) return std::nullopt;

    auto untilSwitch = m_nextSwitch.until();

    if (untilSwitch.isZero()) {
        // switch now
        this->calculateNextSwitch();
        int player = m_detNext ? m_detNext : this->pickNextPlayer();
        m_detNext = 0;

        return SwitcherooSwitchEvent {
            .playerId = player,
            .type = SwitchType::Switch,
        };
    } else if (!m_warned && untilSwitch < *asp::Duration::fromSecs(m_settings.m_warningDelay)) {
        // calculate player and show warning
        m_detNext = this->pickNextPlayer();
        m_warned = true;

        return SwitcherooSwitchEvent {
            .playerId = m_detNext,
            .type = SwitchType::Warning,
        };
    }

    return std::nullopt;
}

void APSController::calculateNextSwitch() {
    m_warned = false;

    float var = std::abs(m_settings.m_intervalVar);
    float base = std::abs(m_settings.m_interval);
    float untilNext = utils::random::generate<float>(base - var, base + var);
    m_nextSwitch = asp::Instant::now() + *asp::Duration::fromSecs(untilNext);
}

int APSController::pickNextPlayer() {
    switch (m_settings.m_cycleAlgo) {
        case APSSelectionAlgorithm::FairRandom: {
            if (m_pqueue.empty()) {
                this->getAllPlayers();
                utils::random::shuffle(m_pqueue);

                // ensure that the same person can't play twice in a row!
                if (m_pqueue.back() == m_activePlayer) {
                    std::swap(m_pqueue.front(), m_pqueue.back());
                }
            }

            int ret = m_pqueue.back();
            m_pqueue.pop_back();
            return ret;
        } break;

        case APSSelectionAlgorithm::Sequential: {
            if (m_pqueue.empty()) {
                this->getAllPlayers();
                std::ranges::sort(m_pqueue);
            }

            int ret = m_pqueue.back();
            m_pqueue.pop_back();
            return ret;
        } break;

        case APSSelectionAlgorithm::Random: {
            // yes it's not efficient but we recollect all players into the vector every time,
            // so that we dont miss people that newly joined
            this->getAllPlayers();

            while (true) {
                int next = m_pqueue[utils::random::generate(0, m_pqueue.size())];
                if (next != m_activePlayer || m_pqueue.size() == 1) return next;
            }
        } break;
    }

    return 0;
}

void APSController::getAllPlayers() {
    m_pqueue = asp::iter::keys(m_gjbgl->m_fields->m_players).collect();
    m_pqueue.push_back(m_pl->m_fields->m_myAccountId);
}

// PlayLayer

bool APSPlayLayer::init(GJGameLevel* level, bool a, bool b) {
    if (!PlayLayer::init(level, a, b)) return false;

    auto& fields = *m_fields.self();
    fields.m_myAccountId = GJAccountManager::get()->m_accountID;
    fields.m_controller.m_pl = this;
    fields.m_controller.m_gjbgl = GlobedGJBGL::get(this);
    fields.m_controlling = RoomManager::get().isOwner();

    fields.m_listener = NetworkManagerImpl::get().listen<msg::LevelDataMessage>([this](const msg::LevelDataMessage& msg) {
        auto& controller = m_fields->m_controller;
        for (auto& event : msg.events) {
            if (event.is<SwitcherooFullStateEvent>()) {
                controller.handleStateEvent(event.as<SwitcherooFullStateEvent>());
            } else if (event.is<SwitcherooSwitchEvent>()) {
                controller.handleSwitchEvent(event.as<SwitcherooSwitchEvent>());
            }
        }
    }, -10);

    this->addEventListener(
        KeybindSettingPressedEventV3(Mod::get(), "keybind-force-switch"),
        [this](Keybind const& keybind, bool down, bool repeat, double time) {
            if (repeat || !down) return;
            m_fields->m_controller.rescheduleNextSwitch(0.f);
        }
    );

    float glowScale = 1.5f;

    fields.m_switchGlow = Build(NineSlice::create("switch-screen-glow.png"_spr))
        .scale(glowScale)
        .zOrder(11)
        .contentSize(CCDirector::get()->getWinSize() / glowScale)
        .visible(false)
        .id("switch-glow"_spr)
        .parent(m_uiLayer)
        .anchorPoint(0.f, 0.f);

    fields.m_switchPreglow = Build(NineSlice::create("switch-screen-preglow.png"_spr))
        .scale(glowScale)
        .zOrder(10)
        .contentSize(CCDirector::get()->getWinSize() / glowScale)
        .visible(false)
        .id("switch-preglow"_spr)
        .parent(m_uiLayer)
        .anchorPoint(0.f, 0.f);

    if (mh::isHackEnabled(mh::MH_FRAME_EXTRAPOLATION)) {
        mh::setHackEnabled(mh::MH_FRAME_EXTRAPOLATION, false);
        PopupManager::get().alert(
            "Globed Warning",
            "<cy>Frame Extrapolation</c> in <cy>Mega Hack</c> is not compatible with this mode, so it has been disabled."
        ).showQueue();
    }

    return true;
}

void APSPlayLayer::destroyPlayer(PlayerObject* player, GameObject* obj) {
    auto& mod = APSModule::get();
    auto& fields = *m_fields.self();
    auto& controller = fields.m_controller;
    auto gameLayer = GlobedGJBGL::get();

    if (!gameLayer || controller.m_meActive || controller.m_activePlayer == 0) {
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

void APSPlayLayer::showSwitchEffect() {
    this->showEffect(false);
}

void APSPlayLayer::showPreSwitchEffect() {
    this->showEffect(true);
}

void APSPlayLayer::showNextPlayerNotification(int id) {
    auto gjbgl = GlobedGJBGL::get(this);
    cue::Icons icons;
    std::string username;

    auto player = gjbgl->getPlayer(id);
    if (player) {
        auto& ddata = player->displayData();
        icons = convertPlayerIcons(ddata.icons);
        username = ddata.username;
    } else if (id == m_fields->m_myAccountId) {
        icons = getPlayerIcons();
        username = "you";
    } else {
        return;
    }

    auto icon = cue::PlayerIcon::create(icons);

    Notification::create(fmt::format("Switching to {} soon!", username), icon, 0.5f)->show();
}

void APSPlayLayer::showEffect(bool presw) {
    auto& fields = *m_fields.self();
    auto node = presw ? fields.m_switchPreglow : fields.m_switchGlow;

    this->hideSwitchEffects();

    node->setOpacity(0);
    node->runAction(
        CCSequence::create(
            CCShow::create(),
            CCFadeIn::create(presw ? 0.01f : 0.f),
            CCDelayTime::create(presw ? 99999.f : 0.4f),
            CCFadeOut::create(0.1f),
            CCHide::create(),
            nullptr
        )
    );
}

void APSPlayLayer::hideSwitchEffects() {
    auto& fields = *m_fields.self();
    fields.m_switchPreglow->setVisible(false);
    fields.m_switchPreglow->stopAllActions();
    fields.m_switchGlow->setVisible(false);
    fields.m_switchGlow->stopAllActions();

}

void APSPlayLayer::handlePlayerDeath(const PlayerDeath& death, RemotePlayer* player) {
    if (player->id() == m_fields->m_controller.m_activePlayer) {
        log::info("Handling death from {}", player->displayData().username);
        g_ignoreNoclip = true;
        GlobedGJBGL::get(this)->killLocalPlayer();
        g_ignoreNoclip = false;
    }
}

bool APSPlayLayer::shouldBlockInput() {
    auto& controller = m_fields->m_controller;
    return !controller.m_meActive && controller.m_activePlayer != 0;
}

void APSPlayLayer::restartSwitchCycle() {
    auto& fields = *m_fields.self();

    // start the game and send full state
    fields.m_controller.restart();
    this->sendFullState(true);
}

void APSPlayLayer::sendFullState(bool restarting) {
    auto& fields = *m_fields.self();

    // collect the game state and send to all users
    SwitcherooFullStateEvent ev{};
    ev.gameActive = fields.m_controller.m_gameActive;
    ev.activePlayer = fields.m_controller.m_activePlayer;
    ev.playerIndication = fields.m_controller.m_settings.m_showNextPlayer;
    ev.restarting = restarting;

    NetworkManagerImpl::get().queueGameEvent(ev);
}

void APSPlayLayer::updateSettings(const APSSettings& settings) {
    auto& set = m_fields->m_controller.m_settings;
    set = settings;

    // validate settings

    // interval must be at least 1 second
    set.m_interval = std::max(set.m_interval, 1.f);

    // variance cannot be higher than the base interval
    set.m_intervalVar = std::min(set.m_intervalVar, set.m_interval - 1.f);

    // warning delay cannot be higher than the base interval
    set.m_warningDelay = std::min(set.m_warningDelay, set.m_interval - 1.f);
}

void APSPlayLayer::handleUpdate(float dt) {
    auto& fields = *m_fields.self();
    auto& controller = fields.m_controller;
    auto self = GlobedGJBGL::get(this);

    if (fields.m_controlling) {
        auto ev = controller.poll();
        if (ev) {
            NetworkManagerImpl::get().queueGameEvent(std::move(*ev));
        }
    }

    bool takeOverCamera = false;
    if (!controller.m_meActive && controller.m_activePlayer != 0) {
        auto rp = self->getPlayer(controller.m_activePlayer);
        if (rp) {
            this->handleUpdateFromRp(m_player1, rp.get(), false);
            this->handleUpdateFromRp(m_player2, rp.get(), true);
            self->setCameraFollowPlayer(rp->player1());
            takeOverCamera = true;

            buttonPress(self, rp->player1()->isHolding(), false);
            buttonPress(self, rp->player2()->isHolding(), true);
        } else {
            log::warn("active player {} not found!", controller.m_activePlayer);
            controller.m_activePlayer = 0;
        }
    }

    if (!takeOverCamera) {
        self->setCameraFollowPlayer(nullptr);
    }

    // takeOverCamera = false;
    setPlayerHidden(m_player1, takeOverCamera);
    setPlayerHidden(m_player2, takeOverCamera);
    setPlayerHidden(self->m_fields->m_ghost.get(), takeOverCamera);
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
    local->m_maybeIsFalling = p1->m_maybeIsFalling;
    local->m_fallSpeed = p1->m_fallSpeed;
    local->m_isOnGround4 = p1->m_isOnGround4;

    // m_player1->m_isRotating = p1->m_isRotating;
    // m_player1->m_isSideways = p1->m_isSideways;
    // m_player1->m_isDashing = p1->m_isDashing;
}

APSPlayLayer* APSPlayLayer::get(GJBaseGameLayer* gjbgl) {
    if (!gjbgl || gjbgl->m_isEditor) {
        gjbgl = globed::get<GameManager>()->m_playLayer;
    }

    return static_cast<APSPlayLayer*>(gjbgl);
}

// GJBGL

void APSGJBGL::handleButton(bool a, int b, bool p1) {
    auto pl = APSPlayLayer::get(this);
    if (!pl || !pl->shouldBlockInput() || g_ignoreButtonBlock) {
        GJBaseGameLayer::handleButton(a, b, p1);

        // log::debug("(APS) Button {} {} {}", a, b, p1);
        auto& b = p1 ? g_p1Held : g_p2Held;
        b = a;
    }
}

// TODO: also fyi player2 field is not flipped
// if like flip 2p controls is enabled
void APSGJBGL::processQueuedButtons(float dt, bool clearInputQueue) {
    for (auto& btn : m_queuedButtons) {
        if (btn.m_isPlayer2) {
            auto& v = m_gameState.m_isDualMode ? g_realP2Held : g_realP1Held;
            v = btn.m_isPush;
        } else {
            g_realP1Held = btn.m_isPush;
        }
    }
    GJBaseGameLayer::processQueuedButtons(dt, clearInputQueue);
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

    auto winSize = globed::get<CCDirector>()->getWinSize();

    auto menu = this->getChildByID("playerlist-menu"_spr);
    if (!menu) {
        log::error("playerlist-menu not found, cannot add aps button!");
        return;
    }

    Build<CCSprite>::create("icon-switcheroo.png"_spr)
        .scale(0.9f)
        .intoMenuItem(+[] {
            if (APSPlayLayer::get()) APSSettingsPopup::create()->show();
        })
        .scaleMult(1.2f)
        .id("btn-switcheroo"_spr)
        .parent(menu);

    menu->updateLayout();
}

void APSPlayerObject::update(float dt) {
    PlayerObject::update(dt);
}

}
