#include "GJBaseGameLayer.hpp"
#include <globed/audio/AudioManager.hpp>
#include <globed/core/RoomManager.hpp>
#include <globed/core/PlayerCacheManager.hpp>
#include <globed/core/SettingsManager.hpp>
#include <globed/core/KeybindsManager.hpp>
#include <globed/core/PopupManager.hpp>
#include <globed/core/FriendListManager.hpp>
#include <globed/util/algo.hpp>
#include <globed/util/gd.hpp>
#include <globed/util/FunctionQueue.hpp>
#include <globed/util/GameState.hpp>
#include <core/CoreImpl.hpp>
#include <core/PreloadManager.hpp>
#include <core/net/NetworkManagerImpl.hpp>

#include <Geode/utils/VMTHookManager.hpp>
#include <UIBuilder.hpp>
#include <asp/time/Instant.hpp>
#include <qunet/util/algo.hpp>
#include <cue/Util.hpp>

using namespace geode::prelude;
using namespace asp::time;

constexpr float VOICE_OVERLAY_PAD_X = 5.f;
constexpr float VOICE_OVERLAY_PAD_Y = 20.f;
constexpr auto EMOTE_COOLDOWN = Duration::fromMillis(2500);

namespace {

class CustomSchedule : public CCObject {
public:
    using Fn = std23::move_only_function<void(globed::GlobedGJBGL*, float)>;

    static CustomSchedule* create(Fn&& fn, float interval, globed::GlobedGJBGL* gjbgl) {
        auto ret = new CustomSchedule;
        ret->m_fn = std::move(fn);
        ret->m_gjbgl = gjbgl;
        ret->autorelease();
        CCScheduler::get()->scheduleSelector(schedule_selector(CustomSchedule::invoke), ret, interval, false);
        return ret;
    }
private:
    Fn m_fn;
    globed::GlobedGJBGL* m_gjbgl;

    CustomSchedule() {}

    void invoke(float dt) {
        m_fn(m_gjbgl, dt);
    }
};

}

namespace globed {

/// Cache for some settings that are often accessed
struct CachedSettings {
    bool selfName = globed::setting<bool>("core.level.self-name");
    bool selfStatusIcons = globed::setting<bool>("core.level.self-status-icons");
    bool quickChat = globed::setting<bool>("core.player.quick-chat-enabled");
    bool voiceChat = globed::setting<bool>("core.audio.voice-chat-enabled");
    bool voiceLoopback = globed::setting<bool>("core.audio.voice-loopback");
    bool friendsOnlyAudio = globed::setting<bool>("core.audio.only-friends");
    bool editorEnabled = globed::setting<bool>("core.editor.enabled");
    bool ghostFollower = globed::setting<bool>("core.dev.ghost-follower");

    void reload() {
        *this = CachedSettings{};
    }
} g_settings;

static std::optional<Instant> g_lastEmoteTime;

static int myAccountId() {
    return cachedSingleton<GJAccountManager>()->m_accountID;
}

void GlobedGJBGL::setupPreInit(GJGameLevel* level, bool editor) {
    auto& fields = *m_fields.self();
    auto& nm = NetworkManagerImpl::get();
    fields.m_editor = editor;

    this->reloadCachedSettings();

    // determine if mulitplayer should be active

    auto ecId = RoomManager::get().getEditorCollabId(level);
    bool isEditorCollab = ecId && ecId->asU64() && ecId->asU64() != (uint64_t)level->m_levelID;

    fields.m_active = nm.isConnected() && (level->m_levelID != 0 || isEditorCollab);
    if (level->m_levelType == GJLevelType::Editor && !g_settings.editorEnabled) {
        fields.m_active = false;
    }

    fields.m_interpolator.setPlatformer(level->isPlatformer());

    if (fields.m_active) {
        RoomManager::get().joinLevel(level);
        CoreImpl::get().onJoinLevel(this, level, editor);
    }
}

void GlobedGJBGL::setupPostInit() {
    auto& fields = *m_fields.self();
    this->setupNecessary();

    if (!fields.m_active) return;

    auto& km = KeybindsManager::get();
    km.refreshBinds();

    // setup everything else
    this->setupAssetLoading();
    this->setupAudio();
    this->setupUpdateLoop();
    this->setupUi();
    this->setupListeners();

    // add ghost player
    fields.m_ghost = std::make_shared<RemotePlayer>(0, this, fields.m_playerNode);
    fields.m_ghost->initData(PlayerDisplayData::getOwn(), false);

    CoreImpl::get().onJoinLevelPostInit(this);
}

void GlobedGJBGL::setupNecessary() {
    auto& fields = *m_fields.self();

    fields.m_pingOverlay = Build<PingOverlay>::create()
        .scale(0.4f)
        .zOrder(11)
        .id("game-overlay"_spr);
    fields.m_pingOverlay->addToLayer(this);

    auto& nm = NetworkManagerImpl::get();
    int levelId = m_level->m_levelID;

    if (!nm.isConnected()) {
        fields.m_pingOverlay->updateWithDisconnected();
    } else if (!fields.m_active) {
        fields.m_pingOverlay->updateWithEditor();
    } else {
        fields.m_pingOverlay->updatePing(nm.getGamePing().millis());
    }
}

void GlobedGJBGL::setupAssetLoading() {
    auto& pm = PreloadManager::get();
    pm.enterContext(PreloadContext::Level);

    if (pm.shouldPreload()) {
        log::info("Preloading assets (deferred)");

        auto start = Instant::now();
        pm.loadEverything();

        log::info("Asset preloading took {} ({} items loaded)", start.elapsed().toString(), pm.getLoadedCount());
    }

    pm.exitContext();
}

void GlobedGJBGL::setupAudio() {
    if (!globed::setting<bool>("core.audio.voice-chat-enabled")) return;

    auto& am = AudioManager::get();
    am.stopAllOutputSources(); // preemptively
    m_fields->m_audioInterval.setInterval(Duration::fromSecsF32(1.f / 30.f).value());

#ifdef GLOBED_VOICE_CAN_TALK
    // set audio device
    am.refreshDevices();
    am.setRecordBufferCapacity(globed::setting<int>("core.audio.buffer-size"));
    auto result = am.startRecordingEncoded([this](const auto& frame) {
        NetworkManagerImpl::get().sendVoiceData(frame);

        if (g_settings.voiceLoopback) {
            m_fields->m_ghost->playVoiceData(frame);
        }
    });
    am.setPassiveMode(true);

    if (!result) {
        log::warn("[Globed] Failed to start recording audio: {}", result.unwrapErr());
        globed::toastError("[Globed] Failed to start recording audio");
    }
#endif

    auto winSize = CCDirector::get()->getWinSize();

    // enable voice proximity?
    m_fields->m_isVoiceProximity = m_level->isPlatformer()
        ? globed::setting<bool>("core.audio.voice-proximity")
        : globed::setting<bool>("core.audio.classic-proximity");

    // schedule voice overlay 1 frame later, when we have a scene
    FunctionQueue::get().queue([self = Ref(this), winSize] {
        bool onTop = globed::setting<bool>("core.audio.overlaying-overlay");
        CCNode* scene = self->getParent();
        CCNode* parent = onTop && scene ? scene : self->m_uiLayer;

        self->m_fields->m_voiceOverlay = Build<VoiceOverlay>::create()
            .parent(parent)
            .visible(globed::setting<bool>("core.level.voice-overlay"))
            .zOrder(onTop ? 20 : 1)
            .pos(winSize.width - VOICE_OVERLAY_PAD_X, VOICE_OVERLAY_PAD_Y)
            .anchorPoint(1.f, 0.f)
            .collect();
    });
}

void GlobedGJBGL::setupUpdateLoop() {
    auto& fields = *m_fields.self();

    // this has to be deferred. why? i don't know! but bugs happen otherwise
    FunctionQueue::get().queue([this] {
        auto self = GlobedGJBGL::get();
        if (!self || !self->active()) {
            return;
        }

        auto& fields = *self->m_fields.self();

        if (auto p = this->getParent()) {
            p->schedule(schedule_selector(GlobedGJBGL::selUpdateProxy), 0.f);
            fields.m_didSchedule = true;
        } else {
            log::warn("Failed to schedule update loop, parent is null, will try again later");
        }

        self->scheduleOnce(schedule_selector(GlobedGJBGL::selPostInitActions), 0.25f);
    });
}

void GlobedGJBGL::setupUi() {
    auto& fields = *m_fields.self();

    Build<CCNode>::create()
        .id("player-node"_spr)
        .zOrder(1500)
        .parent(m_objectLayer)
        .store(fields.m_playerNode);

    fields.m_progressBarContainer = Build<CCNode>::create()
        .id("progress-bar-wrapper"_spr)
        .visible(globed::setting<bool>("core.level.progress-indicators"))
        .zOrder(-1);

    if (auto pl = this->asPlayLayer()) {
        if (pl->m_progressBar) {
            pl->m_progressBar->addChild(fields.m_progressBarContainer);
        }
    }
}

void GlobedGJBGL::setupListeners() {
    auto& fields = *m_fields.self();
    auto& nm = NetworkManagerImpl::get();

    fields.m_levelDataListener = nm.listen<msg::LevelDataMessage>([this](const msg::LevelDataMessage& message) {
        this->onLevelDataReceived(message);
        return ListenerResult::Continue;
    });

    fields.m_voiceListener = nm.listen<msg::VoiceBroadcastMessage>([this](const msg::VoiceBroadcastMessage& message) {
        this->onVoiceDataReceived(message);
        return ListenerResult::Continue;
    });

    fields.m_quickChatListener = nm.listen<msg::QuickChatBroadcastMessage>([this](const msg::QuickChatBroadcastMessage& message) {
        this->onQuickChatReceived(message.accountId, message.quickChatId);
        return ListenerResult::Continue;
    });

    fields.m_mutedListener = nm.listen<msg::ChatNotPermittedMessage>([this](const msg::ChatNotPermittedMessage&) {
        m_fields->m_knownServerMuted = true;
        return ListenerResult::Continue;
    });
}

void GlobedGJBGL::onEnterHook() {
    // when unpausing regularly, this is true, otherwise false
    auto weRunningScene = this->getParent() == CCScene::get();

    if (weRunningScene) {
        CCLayer::onEnter();
        return;
    }

    Loader::get()->queueInMainThread([self = Ref(this)] {
        // i forgot why i don't use `self` here and also apparently GlobedGJBGL::get can be null here for 1 person in the world
        auto l = GlobedGJBGL::get();
        bool isPaused = l ? l->isPaused(false) : self->isPaused(false);

        if (!isPaused) {
            self->CCLayer::onEnter();
        }
    });
}

void GlobedGJBGL::onQuit() {
    auto& am = AudioManager::get();
    am.haltRecording();
    am.stopAllOutputSources();

    auto& fields = *m_fields.self();
    fields.m_quitting = true;

    if (!fields.m_active) {
        return;
    }

    fields.m_active = false;
    CoreImpl::get().onLeaveLevel(this, fields.m_editor);
    RoomManager::get().leaveLevel();
}

void GlobedGJBGL::selUpdateProxy(float dt) {
    if (auto self = GlobedGJBGL::get()) {
        self->active() ? self->selUpdate(dt) : (void)0;
    }
}

void GlobedGJBGL::selUpdate(float tsdt) {
    auto& fields = *m_fields.self();
    if (!fields.m_active) return;

    auto& pcm = PlayerCacheManager::get();
    auto& rm = RoomManager::get();

    float dt = tsdt / CCScheduler::get()->getTimeScale();
    fields.m_timeCounter += dt;

    auto camPos = m_gameState.m_cameraPosition;
    auto cameraDelta = fields.m_cameraTracker.pushMeasurement(fields.m_timeCounter, camPos.x, camPos.y);
    auto cameraVector = fields.m_cameraTracker.getVector();

    // process stuff
    fields.m_interpolator.tick(
        dt,
        CCPoint{(float) cameraDelta.first, (float) cameraVector.second},
        CCPoint{(float) cameraVector.first, (float) cameraVector.second}
    );

    fields.m_unknownPlayers.clear();

    auto camState = this->getCameraState();

    for (auto& it : fields.m_players) {
        int playerId = it.first;
        auto& player = it.second;

        if (!fields.m_interpolator.hasPlayer(playerId)) {
            log::error("Interpolator is missing a player: {}", playerId);
            continue;
        }

        // if the player has left the level, remove them
        if (fields.m_interpolator.isPlayerStale(playerId, fields.m_lastServerUpdate)) {
            this->handlePlayerLeave(playerId);
            continue;
        }

        OutFlags flags{};
        auto& vstate = fields.m_interpolator.getPlayerState(playerId, flags);
        player->update(vstate, camState, flags, fields.m_playersHidden);

        // if we don't know player's data yet (username, icons, etc.), request it
        bool dataInit = player->isDataInitialized();
        bool dataOutdated = player->isDataOutdated();

        if (!dataInit || dataOutdated) {
            bool inLayer1 = pcm.hasInLayer1(playerId);
            bool inAny = pcm.has(playerId);

            // if not in layer 1, always request more up to date data
            if (!inLayer1) {
                fields.m_unknownPlayers.push_back(playerId);
            }

            // if not initialized, use whatever we have
            if (!dataInit && inAny) {
                player->initData(*pcm.get(playerId), !inLayer1);
            } else if (dataOutdated && inLayer1) {
                // if outdated and we received layer 1 data, update from there
                player->initData(*pcm.get(playerId), false);
            }
        } else if (!player->isTeamInitialized() && rm.getSettings().teams) {
            if (auto teamId = rm.getTeamIdForPlayer(playerId)) {
                player->updateTeam(*teamId);
            } else {
                log::debug("player {} has unknown team", playerId);
            }
        }
    }

    // update audio
    if (fields.m_audioInterval.tick()) {
        AudioManager::get().updatePlayback(camState.cameraCenter(), fields.m_isVoiceProximity);
    }

    // the server might not send any updates if there are no players on the level,
    // if we receive no response for a while, assume all players have left
    if (fields.m_timeCounter - fields.m_lastServerUpdate > 1.5f && fields.m_players.size() <= 2) {
        for (auto& it : fields.m_players) {
            int playerId = it.first;
            this->handlePlayerLeave(playerId);
        }
    }

    // refresh teams if needed
    if (rm.getSettings().teams) {
        if (fields.m_timeCounter - fields.m_lastTeamRefresh > 10.f) {
            NetworkManagerImpl::get().sendGetTeamMembers();
            fields.m_lastTeamRefresh = fields.m_timeCounter;
        }
    }

    // readjust send interval if needed
    if (fields.m_sendInterval.interval().isZero()) {
        auto& nm = NetworkManagerImpl::get();
        auto tr = nm.getGameTickrate();

        if (tr != 0) {
            float val = 1.f / std::min<float>(240.f, tr);
            auto interval = Duration::fromSecsF32(val).value();
            fields.m_sendInterval.setInterval(interval);
            fields.m_sendThrottledInterval.setInterval(interval * 8.f);
        }
    }

    // send player data to the server
    auto state = this->getPlayerState();
    auto& sendInterval = fields.m_throttleUpdates
        ? fields.m_sendThrottledInterval
        : fields.m_sendInterval;

    if (sendInterval.tick()) {
        this->sendPlayerData(state);
    }

    // update ghost player
    OutFlags ghostFlags{};
    state.accountId = 0;
    fields.m_ghost->update(state, camState, ghostFlags, !g_settings.ghostFollower);

    fields.m_periodicalDelta += dt;
    if (fields.m_periodicalDelta >= 0.25f) {
        this->selPeriodicalUpdate(fields.m_periodicalDelta);
        fields.m_periodicalDelta = 0.f;
    }
}

void GlobedGJBGL::selPeriodicalUpdate(float dt) {
    auto& fields = *m_fields.self();

    // show a little alert icon in the corner if there's any popups waiting to be shown
    bool anyPopups = PopupManager::get().hasPendingPopups();
    if (anyPopups != fields.m_showingNoticeAlert) {
        fields.m_showingNoticeAlert = anyPopups;
        this->setNoticeAlertActive(anyPopups);
    }

    if (!fields.m_active) {
        fields.m_pingOverlay->updateWithDisconnected();
        return;
    }

    fields.m_pingOverlay->updatePing(NetworkManagerImpl::get().getGamePing().millis());

    // check if the user is afk
    auto state = getCurrentGameState();
    if (state != GameState::Active) {
        // stop recording audio if the user is afk
        this->pauseVoiceRecording();
    }

    // send data less often if the game is inactive or there are no other players
    bool prevThrottle = fields.m_throttleUpdates;
    fields.m_throttleUpdates = state == GameState::Closed || fields.m_players.empty();

    if (prevThrottle != fields.m_throttleUpdates) {
        log::debug("updating data send interval to {}", fields.m_throttleUpdates ? "throttled" : "normal");
    }
}

void GlobedGJBGL::selPostInitActions(float dt) {
    auto& fields = *m_fields.self();

    if (!fields.m_didSchedule) {
        if (auto p = this->getParent()) {
            p->schedule(schedule_selector(GlobedGJBGL::selUpdateProxy), 0.f);
            fields.m_didSchedule = true;
        } else {
            log::error("Failed to schedule update loop, parent is null again");
        }
    }
}

void GlobedGJBGL::sendPlayerData(const PlayerState& state) {
    auto& nm = NetworkManagerImpl::get();
    // do not do anything if we aren't connected
    if (!nm.isGameConnected()) return;

    auto& fields = *m_fields.self();
    fields.m_totalSentPackets++;

    std::vector<int> toRequest;
    float sinceRequest = fields.m_timeCounter - fields.m_lastDataRequest;

    // only request data if there's no in flight request or more than 1 second has passed since one was made (likely lost)
    if (fields.m_lastDataRequest == 0.f || sinceRequest > 1.f) {
        toRequest.reserve(std::min<size_t>(fields.m_unknownPlayers.size(), 64));

        for (int player : m_fields->m_unknownPlayers) {
            if (player <= 0 || toRequest.size() >= 64) {
                continue;
            }

            toRequest.push_back(player);
        }

        fields.m_lastDataRequest = fields.m_timeCounter;

        // TODO: technically there's a possibility for a "ghost player" where we think they are on the level, but the server is not aware of them,
        // this will cause them to be sent every single time (as the server will never send their data). not sure how to handle this yet.
    }

    // get camera position and radius
    auto camState = this->getCameraState();
    auto coverage = camState.cameraCoverage();

    CCPoint camCenter = camState.cameraOrigin + coverage / 2.f;

    float camRadius = fields.m_noGlobalCulling
        ? INFINITY
        : std::max(coverage.width, coverage.height) / 2.f * 2.75f;

    // extend the radius to the proximity limit
    camRadius = std::max(camRadius, PROXIMITY_AUDIO_LIMIT);

    nm.sendPlayerState(state, toRequest, camCenter, camRadius);
}

PlayerState GlobedGJBGL::getPlayerState() {
    auto& fields = *m_fields.self();

    PlayerState out{};
    out.accountId = myAccountId();
    out.timestamp = fields.m_timeCounter;
    out.frameNumber = 0;
    out.deathCount = fields.m_deathCount;

    // this function (getCurrentPercent) only exists in playlayer and not the editor, so reimpl it
    auto getPercent = [&](){
        float percent;

        if (m_level->m_timestamp > 0) {
            percent = static_cast<float>(m_gameState.m_currentProgress) / m_level->m_timestamp * 100.f;
        } else {
            percent = m_player1->getPosition().x / m_levelLength * 100.f;
        }

        if (percent >= 100.f) {
            return 100.f;
        } else if (percent <= 0.f) {
            return 0.f;
        } else {
            return percent;
        }
    };

    double progress = (double)getPercent() / 100.0;
    if (std::isnan(progress) || std::isinf(progress)) {
        progress = 0.0;
    }

    out.percentage = static_cast<uint16_t>(std::floor(progress * 65535.0));
    out.isDead = m_player1->m_isDead || m_player2->m_isDead;
    out.isPaused = this->isPaused();
    out.isPracticing = m_isPracticeMode;
    out.isInEditor = this->isEditor();
    out.isEditorBuilding = out.isInEditor && m_playbackMode == PlaybackMode::Not;
    out.isLastDeathReal = fields.m_lastLocalDeathReal;

    auto getPlayerObjState = [this, &fields](PlayerObject* obj, PlayerObjectData& out, bool player1){
        using enum PlayerIconType;

        PlayerIconType iconType = Cube;
        if (obj->m_isShip) iconType = m_level->isPlatformer() ? Jetpack : Ship;
        else if (obj->m_isBall) iconType = Ball;
        else if (obj->m_isBird) iconType = Ufo;
        else if (obj->m_isDart) iconType = Wave;
        else if (obj->m_isRobot) iconType = Robot;
        else if (obj->m_isSpider) iconType = Spider;
        else if (obj->m_isSwing) iconType = Swing;
        out.iconType = iconType;

        auto pobjInner = obj->getChildrenExt()[0];
        out.position = obj->getPosition();
        out.rotation = globed::normalizeAngle(obj->getRotation());

        out.isVisible = obj->isVisible();
        out.isLookingLeft = obj->m_isGoingLeft;
        // wtf was this for?
        // out.isUpsideDown = (iconType == Swing || iconType == Cube) ? obj->m_isUpsideDown : pobjInner->getScaleY() == -1.0f;
        out.isUpsideDown = obj->m_isUpsideDown;
        out.isDashing = obj->m_isDashing;
        out.isMini = obj->m_vehicleSize != 1.0f;
        out.isGrounded = obj->m_isOnGround;
        out.isStationary = m_level->isPlatformer() ? std::abs(obj->m_platformerXVelocity) < 0.1 : false;
        out.isFalling = obj->m_yVelocity < 0.0f;
        out.isRotating = obj->m_isRotating;
        out.isSideways = obj->m_isSideways;
        out.didJustJump = (player1 ? fields.m_didJustJump1 : fields.m_didJustJump2).take();

        if (fields.m_sendExtData) {
            // gather some extra data
            auto ed = ExtendedPlayerData{};
            ed.velocityX = obj->m_platformerXVelocity;
            ed.velocityY = obj->m_yVelocity;
            ed.accelerating = obj->m_isAccelerating;
            ed.acceleration = obj->m_accelerationOrSpeed;
            ed.fallStartY = obj->m_fallStartY;
            ed.isOnGround2 = obj->m_isOnGround2;
            ed.gravityMod = obj->m_gravityMod;
            ed.gravity = obj->m_gravity;
            ed.touchedPad = obj->m_touchedPad;

            out.extData = ed;
        }
    };

    out.player1 = PlayerObjectData{};
    getPlayerObjState(m_player1, *out.player1, true);

    if (m_gameState.m_isDualMode) {
        out.player2 = PlayerObjectData{};
        getPlayerObjState(m_player2, *out.player2, false);
    }

    return out;
}

bool GlobedGJBGL::isPaused(bool checkCurrent) {
    if (this->isEditor()) {
        return m_playbackMode == PlaybackMode::Paused;
    }

    if (checkCurrent && !this->isCurrentPlayLayer()) {
        return false;
    }

    for (CCNode* child : CCArrayExt<CCNode*>(this->getParent()->getChildren())) {
        if (typeinfo_cast<PauseLayer*>(child)) {
            return true;
        }
    }

    return false;
}

bool GlobedGJBGL::isEditor() {
    // return m_fields->m_editor;
    return m_isEditor;
}

bool GlobedGJBGL::isCurrentPlayLayer() {
    auto pl = CCScene::get()->getChildByType<PlayLayer>(0);
    return static_cast<GJBaseGameLayer*>(pl) == this;
}

bool GlobedGJBGL::isManuallyResetting() {
    return m_fields->m_manualReset;
}

bool GlobedGJBGL::isSafeMode() {
    return m_fields->m_safeMode;
}

bool GlobedGJBGL::isQuitting() {
    return m_fields->m_quitting;
}

void GlobedGJBGL::handlePlayerJoin(int playerId) {
    auto& sm = SettingsManager::get();
    auto& fields = *m_fields.self();

#ifdef GLOBED_DEBUG
    log::debug("Player joined: {}", playerId);
#endif

    if (fields.m_players.contains(playerId)) {
        return;
    }

    auto rp = std::make_shared<RemotePlayer>(playerId, this, fields.m_playerNode);
    rp->setForceHide(sm.isPlayerHidden(playerId));
    fields.m_players.emplace(playerId, std::move(rp));
    fields.m_interpolator.addPlayer(playerId);

    CoreImpl::get().onPlayerJoin(this, playerId);
}

void GlobedGJBGL::handlePlayerLeave(int playerId) {
    auto& am = AudioManager::get();

    auto& fields = *m_fields.self();

#ifdef GLOBED_DEBUG
    log::debug("Player left: {}", playerId);
#endif

    if (!fields.m_players.contains(playerId)) {
        return;
    }

    auto& player = fields.m_players.at(playerId);
    player->stopVoiceStream();
    CoreImpl::get().onPlayerLeave(this, playerId);

    fields.m_players.erase(playerId);
    fields.m_interpolator.removePlayer(playerId);
    PlayerCacheManager::get().evictToLayer2(playerId);
}

void GlobedGJBGL::handleLocalPlayerDeath(PlayerObject* obj) {
    auto& fields = *m_fields.self();

    fields.m_deathCount++;
    fields.m_lastLocalDeathReal = !fields.m_isFakingDeath;
}

void GlobedGJBGL::setPermanentSafeMode() {
    auto& fields = *m_fields.self();

    fields.m_permanentSafeMode = true;
    fields.m_safeMode = true;
}

void GlobedGJBGL::killLocalPlayer(bool fake) {
    auto& fields = *m_fields.self();
    log::debug("Killing player");

    fields.m_isFakingDeath = fake;

    this->destroyPlayer(m_player1, nullptr);

    fields.m_isFakingDeath = false;
}

void GlobedGJBGL::resetSafeMode() {
    auto& fields = *m_fields.self();
    fields.m_safeMode = false;
}

void GlobedGJBGL::toggleSafeMode() {
    auto& fields = *m_fields.self();
    fields.m_safeMode = false;
}

void GlobedGJBGL::pausedUpdate(float dt) {
    auto& fields = *m_fields.self();

    // unpause dash effects and death effects
    int tag1 = SPIDER_DASH_CIRCLE_WAVE_TAG;
    int tag2 = SPIDER_DASH_SPRITE_TAG;
    int tag3 = DEATH_EFFECT_TAG;

    for (auto* child : m_objectLayer->getChildrenExt()) {
        int ctag = child->getTag();
        if (ctag == tag1 || ctag == tag2 || ctag == tag3) {
            child->onEnter();
        }
    }

    for (auto* child : fields.m_playerNode->getChildrenExt()) {
        int ctag = child->getTag();
        bool resume =
            ctag == tag1 || ctag == tag2 || ctag == tag3
            || typeinfo_cast<EmoteBubble*>(child)
            || typeinfo_cast<ExplodeItemNode*>(child);

        if (resume) {
            child->onEnter();
        }
    }
}

PlayLayer* GlobedGJBGL::asPlayLayer() {
    if (!m_isEditor) {
        return static_cast<PlayLayer*>(static_cast<GJBaseGameLayer*>(this));
    } else {
        return nullptr;
    }
}

GlobedGJBGL* GlobedGJBGL::get(GJBaseGameLayer* base) {
    if (!base) {
        base = cachedSingleton<GameManager>()->m_gameLayer;
    }

    return static_cast<GlobedGJBGL*>(base);
}

bool GlobedGJBGL::active() {
    return m_fields->m_active;
}

CameraDirection GlobedGJBGL::getCameraDirection() {
    float angle = -m_gameState.m_cameraAngle;

    float radians = angle * M_PI / 180.f;
    CCPoint vec{std::sin(radians), std::cos(radians)};

    return CameraDirection{
        .vector = vec,
        .angle = angle,
    };
}

GameCameraState GlobedGJBGL::getCameraState() {
    GameCameraState state{};
    state.visibleOrigin = CCPoint{0.f, 0.f};
    state.visibleCoverage = cachedSingleton<CCDirector>()->getWinSize();
    state.cameraOrigin = m_gameState.m_cameraPosition;
    state.zoom = m_objectLayer->getScale();

    return state;
}

std::shared_ptr<RemotePlayer> GlobedGJBGL::getPlayer(int playerId) {
    auto& players = m_fields->m_players;

    auto it = players.find(playerId);

    return it == players.end() ? nullptr : it->second;
}

void GlobedGJBGL::recordPlayerJump(bool p1) {
    auto& fields = *m_fields.self();
    (p1 ? fields.m_didJustJump1 : fields.m_didJustJump2) = true;
}

bool GlobedGJBGL::shouldLetMessageThrough(int playerId) {
    auto& sm = SettingsManager::get();
    auto& flm = FriendListManager::get();

    if (sm.isPlayerBlacklisted(playerId)) return false;
    if (sm.isPlayerWhitelisted(playerId)) return true;

    if (g_settings.friendsOnlyAudio && !flm.isFriend(playerId)) return false;

    return true;
}

bool GlobedGJBGL::isSpeaking(int playerId) {
    if (!g_settings.voiceChat) return false;

    auto it = m_fields->m_players.find(playerId);
    if (it == m_fields->m_players.end()) {
        return false;
    }

    auto stream = it->second->getVoiceStream();
    return stream && !stream->isStarving();
}

void GlobedGJBGL::setNoticeAlertActive(bool active) {
    auto& fields = *m_fields.self();

    if (m_isEditor) {
        // todo
        return;
    }

    // Add the alert if it does not exist
    if (!fields.m_noticeAlert) {
        auto pbm = this->m_uiLayer->getChildByID("pause-button-menu");
        if (!pbm) {
            log::warn("pause-button-menu not found, not toggling notice alert");
            return;
        }

        Build<CCSprite>::createSpriteName("geode.loader/info-alert.png")
            .scale(0.45f)
            .opacity(255)
            .pos(8.f, 8.f)
            .id("notice-alert"_spr)
            .parent(pbm)
            .store(fields.m_noticeAlert);

        fields.m_noticeAlert->runAction(
            CCRepeatForever::create(
                CCSequence::create(
                    CCFadeTo::create(0.65f, 150),
                    CCFadeTo::create(0.65f, 255),
                    nullptr
                )
            )
        );
    }

    if (fields.m_noticeAlert) {
        fields.m_noticeAlert->setVisible(active);
    }
}

void GlobedGJBGL::reloadCachedSettings() {
    g_settings.reload();
}

void GlobedGJBGL::toggleCullingEnabled(bool culling) {
    m_fields->m_noGlobalCulling = !culling;
}

void GlobedGJBGL::toggleExtendedData(bool extended) {
    m_fields->m_sendExtData = extended;
}

void GlobedGJBGL::toggleHidePlayers() {
    auto& fields = *m_fields.self();
    fields.m_playersHidden = !fields.m_playersHidden;

    Notification::create(
        fields.m_playersHidden ? "All Players Hidden" : "All Players Visible",
        NotificationIcon::Success,
        0.2f
    )->show();
}

void GlobedGJBGL::toggleDeafen() {
    bool& deafen = m_fields->m_deafened;
    deafen = !deafen;

    if (globed::setting<bool>("core.audio.deafen-notification")) {
        globed::toast(
            CCSprite::create(deafen ? "deafen-icon-on.png"_spr : "deafen-icon-off.png"_spr),
            0.2f,
            deafen ? "Deafened Voice Chat" : "Undeafened Voice Chat"
        );
    }

    AudioManager::get().setDeafen(deafen);
}

void GlobedGJBGL::resumeVoiceRecording() {
#ifdef GLOBED_VOICE_CAN_TALK
    auto& am = AudioManager::get();

    if (!g_settings.voiceChat || am.getDeafen()) {
        return;
    }

    log::debug("Resuming voice recording");
    AudioManager::get().resumePassiveRecording();
#endif
}

void GlobedGJBGL::pauseVoiceRecording() {
#ifdef GLOBED_VOICE_CAN_TALK
    AudioManager::get().pausePassiveRecording();
#endif
}

void GlobedGJBGL::customSchedule(const std::string& id, std23::move_only_function<void(GlobedGJBGL*, float)>&& f, float interval) {
    this->customSchedule(id, interval, std::move(f));
}

void GlobedGJBGL::customSchedule(const std::string& id, float interval, std23::move_only_function<void(GlobedGJBGL*, float)>&& f) {
    auto sched = CustomSchedule::create(std::move(f), interval, this);
    this->setUserObject(id, sched);
}

void GlobedGJBGL::customUnschedule(const std::string& id) {
    this->setUserObject(id, nullptr);

    auto& fields = *m_fields.self();
    fields.m_customSchedules.erase(std::find_if(fields.m_customSchedules.begin(), fields.m_customSchedules.end(), [&](const auto& s) {
        return s == id;
    }));
}

void GlobedGJBGL::customUnscheduleAll() {
    auto& fields = *m_fields.self();
    for (auto& id : fields.m_customSchedules) {
        this->setUserObject(id, nullptr);
    }

    fields.m_customSchedules.clear();
}

void GlobedGJBGL::onLevelDataReceived(const msg::LevelDataMessage& message) {
    auto& fields = *m_fields.self();

    fields.m_lastServerUpdate = fields.m_timeCounter;

    for (auto& player : message.players) {
        if (player.accountId <= 0) continue;

        if (!fields.m_players.contains(player.accountId)) {
            this->handlePlayerJoin(player.accountId);
        }

        fields.m_interpolator.updatePlayer(player, fields.m_lastServerUpdate);
    }

    for (auto& dd : message.displayDatas) {
        if (dd.accountId <= 0) continue; // should never happen?

        PlayerCacheManager::get().insert(dd.accountId, dd);
    }

    if (!message.displayDatas.empty()) {
        fields.m_lastDataRequest = 0.f; // reset the time, so that we can request more players
    }
}

void GlobedGJBGL::onVoiceDataReceived(const msg::VoiceBroadcastMessage& message) {
    auto& am = AudioManager::get();

    if (am.getDeafen() || !g_settings.voiceChat) {
        return;
    }

    if (!this->shouldLetMessageThrough(message.accountId)) {
        return;
    }

    if (auto player = this->getPlayer(message.accountId)) {
        player->playVoiceData(message.frame);
    }
}

void GlobedGJBGL::onQuickChatReceived(int accountId, uint32_t quickChatId) {
    if (!g_settings.quickChat) {
        return;
    }

    if (!this->shouldLetMessageThrough(accountId)) {
        return;
    }

    auto& fields = *m_fields.self();

    auto it = fields.m_players.find(accountId);
    if (it == fields.m_players.end()) {
        return;
    }

    it->second->player1()->playEmote(quickChatId);
}

bool GlobedGJBGL::playSelfEmote(uint32_t id) {
    if (!g_settings.quickChat) {
        return false;
    }

    // check if we are on cooldown
    auto now = Instant::now();
    if (g_lastEmoteTime && now.durationSince(*g_lastEmoteTime) < EMOTE_COOLDOWN) {
        return false;
    }
    g_lastEmoteTime = now;

    auto& fields = *m_fields.self();
    fields.m_ghost->player1()->playEmote(id);

    NetworkManagerImpl::get().sendQuickChat(id);

    return true;
}

bool GlobedGJBGL::playSelfFavoriteEmote(uint32_t which) {
    auto emote = globed::value<uint32_t>(fmt::format("core.ui.emote-slot-{}", which)).value_or(0);

    if (emote != 0) {
        return this->playSelfEmote(emote);
    }
    return false;
}

}
