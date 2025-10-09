#include "GJBaseGameLayer.hpp"
#include <globed/audio/AudioManager.hpp>
#include <globed/core/RoomManager.hpp>
#include <globed/core/PlayerCacheManager.hpp>
#include <globed/core/SettingsManager.hpp>
#include <globed/core/KeybindsManager.hpp>
#include <globed/core/PopupManager.hpp>
#include <globed/util/algo.hpp>
#include <globed/util/gd.hpp>
#include <core/CoreImpl.hpp>
#include <core/PreloadManager.hpp>
#include <core/net/NetworkManagerImpl.hpp>

#include <UIBuilder.hpp>
#include <asp/time/Instant.hpp>
#include <qunet/util/algo.hpp>

using namespace geode::prelude;
using namespace asp::time;

namespace {

class CustomSchedule : public CCObject {
public:
    using Fn = std::function<void(globed::GlobedGJBGL*, float)>;

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

void GlobedGJBGL::setupPreInit(GJGameLevel* level, bool editor) {
    // TODO: editor stuff
    auto& fields = *m_fields.self();
    auto& nm = NetworkManagerImpl::get();
    fields.m_editor = editor;

    // determine if mulitplayer should be active
    fields.m_active = nm.isConnected() && level->m_levelID != 0;

    fields.m_interpolator.setPlatformer(level->isPlatformer());

    // TODO: disable multiplayer in editor depending on settings

    if (fields.m_active) {
        RoomManager::get().joinLevel(level->m_levelID, level->isPlatformer());
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

    CoreImpl::get().onJoinLevelPostInit(this);
}

void GlobedGJBGL::setupNecessary() {
    // TODO: ping overlay
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

#ifdef GLOBED_VOICE_CAN_TALK
    // set audio device
    am.refreshDevices();
    am.setRecordBufferCapacity(globed::setting<int>("core.audio.buffer-size"));
    auto result = am.startRecordingEncoded([this](const auto& frame) {
        NetworkManagerImpl::get().sendVoiceData(frame);

        if (globed::setting<bool>("core.audio.voice-loopback")) {
            log::debug("Playing loopback voice frame");
            auto& am = AudioManager::get();

            if (auto err = am.playFrameStreamed(-1, frame).err()) {
                log::warn("Failed to play loopback voice frame: {}", err);
            }
        }
    });
    am.setPassiveMode(true);
    am.setGlobalPlaybackVolume(globed::setting<float>("core.audio.playback-volume"));

    if (!result) {
        log::warn("[Globed] Failed to start recording audio: {}", result.unwrapErr());
        globed::toastError("[Globed] Failed to start recording audio");
    }
#endif

    // TODO: setup voice overlay here..
}

void GlobedGJBGL::setupUpdateLoop() {
    auto& fields = *m_fields.self();

    // this has to be deferred. why? i don't know! but bugs happen otherwise
    Loader::get()->queueInMainThread([this] {
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

    fields.m_selfProgressIcon = Build<ProgressIcon>::create()
        .parent(fields.m_progressBarContainer)
        .id("self-player-progress"_spr);

    auto gm = globed::cachedSingleton<GameManager>();

    fields.m_selfProgressIcon->updateIcons(globed::getPlayerIcons());
    fields.m_selfProgressIcon->setForceOnTop(true);

    fields.m_selfStatusIcons = Build(PlayerStatusIcons::create(255))
        .parent(fields.m_playerNode)
        .id("self-player-status-icons"_spr);

    fields.m_selfNameLabel = Build(NameLabel::create(GJAccountManager::get()->m_username.c_str(), "chatFont.fnt"))
        .opacity(globed::setting<float>("core.player.name-opacity") * 255.f)
        .pos(0.f, 28.f)
        .parent(fields.m_playerNode)
        .id("self-player-name"_spr);
    fields.m_selfNameLabel->setShadowEnabled(true);
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

    fields.m_mutedListener = nm.listen<msg::ChatNotPermittedMessage>([this](const msg::ChatNotPermittedMessage&) {
        m_fields->m_knownServerMuted = true;
        return ListenerResult::Continue;
    });
}

void GlobedGJBGL::onQuit() {
    auto& am = AudioManager::get();
    am.haltRecording();
    am.stopAllOutputStreams();

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

    // update progress indicator
    if (!m_isEditor) {
        float perc = static_cast<PlayLayer*>(static_cast<GJBaseGameLayer*>(this))->getCurrentPercent() / 100.f;
        fields.m_selfProgressIcon->updatePosition(perc, m_isPracticeMode);
    }

    fields.m_unknownPlayers.clear();

    auto camState = this->getCameraState();

    for (auto& it : fields.m_players) {
        int playerId = it.first;
        auto& player = it.second;

        if (!fields.m_interpolator.hasPlayer(playerId)) {
            log::error("Interpolator is missing a player: {}", playerId);
            continue;
        }

        OutFlags flags;
        auto& vstate = fields.m_interpolator.getPlayerState(playerId, flags);
        player->update(vstate, camState, fields.m_playersHidden);

        // if the player just died, handle the death
        if (flags.death.has_value()) {
            player->handleDeath(*flags.death);
            CoreImpl::get().onPlayerDeath(this, player.get(), *flags.death);
        }

        // if the player teleported, play a spider dash animation
        if (flags.spiderP1) {
            player->handleSpiderTp(*flags.spiderP1, true);
        } else if (flags.spiderP2) {
            player->handleSpiderTp(*flags.spiderP2, false);
        }

        // if the player has left the level, remove them
        if (fields.m_interpolator.isPlayerStale(playerId, fields.m_lastServerUpdate)) {
            this->handlePlayerLeave(playerId);
            continue;
        }

        // if we don't know player's data yet (username, icons, etc.), request it
        if (!player->isDataInitialized()) {
            if (pcm.has(playerId)) {
                player->initData(*pcm.get(playerId));
            } else {
                fields.m_unknownPlayers.push_back(playerId);
            }
        } else if (!player->isTeamInitialized() && rm.getSettings().teams) {
            if (auto teamId = rm.getTeamIdForPlayer(playerId)) {
                player->updateTeam(*teamId);
            } else {
                log::debug("player {} has unknown team", playerId);
            }
        }
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
    if (fields.m_sendDataInterval == 0.f) {
        auto& nm = NetworkManagerImpl::get();
        auto tr = nm.getGameTickrate();

        if (tr != 0) {
            float interval = 1.f / (float)tr;
            fields.m_sendDataInterval = interval;
        }
    } else {
        // send player data to the server
        if (fields.m_timeCounter - fields.m_lastDataSend >= fields.m_sendDataInterval) {
            fields.m_lastDataSend += fields.m_sendDataInterval;
            this->selSendPlayerData(dt);
        }
    }

    // update position for self icons / username
    PlayerStatusFlags flags{};
    flags.speaking = AudioManager::get().isPassiveRecording();
    flags.speakingMuted = flags.speaking && fields.m_knownServerMuted;

    bool showSelfName = globed::setting<bool>("core.level.self-name");
    bool showSelfIcons = flags.speaking && globed::setting<bool>("core.ui.self-status-icons");

    if (showSelfIcons) {
        fields.m_selfStatusIcons->setVisible(true);
        fields.m_selfStatusIcons->updateStatus(flags);
        fields.m_selfStatusIcons->setPosition({
            m_player1->getPosition() + CCPoint{0.f, showSelfName ? 43.f : 28.f} // TODO
        });
    } else {
        fields.m_selfStatusIcons->setVisible(false);
    }

    if (showSelfName) {
        fields.m_selfNameLabel->setVisible(true);
        fields.m_selfNameLabel->setPosition({
            m_player1->getPosition() + CCPoint{0.f, 28.f}
        });
    } else {
        fields.m_selfNameLabel->setVisible(false);
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

void GlobedGJBGL::selSendPlayerData(float dt) {
    auto& fields = *m_fields.self();
    fields.m_totalSentPackets++;

    auto state = this->getPlayerState();
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

    NetworkManagerImpl::get().sendPlayerState(state, toRequest, camCenter, camRadius);
}

PlayerState GlobedGJBGL::getPlayerState() {
    auto& fields = *m_fields.self();

    PlayerState out{};
    out.accountId = globed::cachedSingleton<GJAccountManager>()->m_accountID;
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

    auto getPlayerObjState = [this, &fields](PlayerObject* obj, PlayerObjectData& out){
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

        auto pobjInner = static_cast<CCNode*>(obj->getChildren()->objectAtIndex(0));
        out.position = obj->getPosition();
        out.rotation = globed::normalizeAngle(obj->getRotation());

        out.isVisible = obj->isVisible();
        out.isLookingLeft = obj->m_isGoingLeft;
        // TODO: wtf was this for?
        // out.isUpsideDown = (iconType == Swing || iconType == Cube) ? obj->m_isUpsideDown : pobjInner->getScaleY() == -1.0f;
        out.isUpsideDown = obj->m_isUpsideDown;
        out.isDashing = obj->m_isDashing;
        out.isMini = obj->m_vehicleSize != 1.0f;
        out.isGrounded = obj->m_isOnGround;
        out.isStationary = m_level->isPlatformer() ? std::abs(obj->m_platformerXVelocity) < 0.1 : false;
        out.isFalling = obj->m_yVelocity < 0.0f;
        // TODO: set didJustJump
        out.isRotating = obj->m_isRotating;
        out.isSideways = obj->m_isSideways;

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
    getPlayerObjState(m_player1, *out.player1);

    if (m_gameState.m_isDualMode) {
        out.player2 = PlayerObjectData{};
        getPlayerObjState(m_player2, *out.player2);
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
    auto& fields = *m_fields.self();

#ifdef GLOBED_DEBUG
    log::debug("Player joined: {}", playerId);
#endif

    if (fields.m_players.contains(playerId)) {
        return;
    }

    auto rp = std::make_unique<RemotePlayer>(playerId, this, fields.m_playerNode);
    fields.m_players.emplace(playerId, std::move(rp));
    fields.m_interpolator.addPlayer(playerId);

    CoreImpl::get().onPlayerJoin(this, playerId);
}

void GlobedGJBGL::handlePlayerLeave(int playerId) {
    auto& am = AudioManager::get();
    am.stopOutputStream(playerId);

    auto& fields = *m_fields.self();

#ifdef GLOBED_DEBUG
    log::debug("Player left: {}", playerId);
#endif

    if (!fields.m_players.contains(playerId)) {
        return;
    }

    auto& player = fields.m_players.at(playerId);
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
    // unpause dash effects and death effects
    for (auto* child : CCArrayExt<CCNode*>(m_objectLayer->getChildren())) {
        int tag1 = SPIDER_DASH_CIRCLE_WAVE_TAG;
        int tag2 = SPIDER_DASH_SPRITE_TAG;
        int tag3 = DEATH_EFFECT_TAG;

        int ctag = child->getTag();

        if (ctag == tag1 || ctag == tag2 || ctag == tag3) {
            child->resumeSchedulerAndActions();
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
        base = GJBaseGameLayer::get();
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

RemotePlayer* GlobedGJBGL::getPlayer(int playerId) {
    auto& players = m_fields->m_players;

    auto it = players.find(playerId);

    return it == players.end() ? nullptr : it->second.get();
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

    AudioManager::get().setDeafen(deafen);
}

void GlobedGJBGL::resumeVoiceRecording() {
    log::debug("Resuming voice recording");
    AudioManager::get().resumePassiveRecording();
}

void GlobedGJBGL::pauseVoiceRecording() {
    AudioManager::get().pausePassiveRecording();
}

void GlobedGJBGL::customSchedule(const std::string& id, std::function<void(GlobedGJBGL*, float)>&& f, float interval) {
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

    if (am.getDeafen() || !globed::setting<bool>("core.audio.voice-chat-enabled")) {
        return;
    }

    // TODO: allow muting a player, reject packet here

    auto res = am.playFrameStreamed(message.accountId, message.frame);
    if (!res) {
        log::warn("Failed to play voice data from {}: {}", message.accountId, res.unwrapErr());
        return;
    }

    float vol = this->calculateVolumeFor(message.accountId);
    am.setStreamVolume(message.accountId, vol);
}

float GlobedGJBGL::calculateVolumeFor(int playerId) {
    // how many units before the voice disappears
    constexpr float PROXIMITY_VOICE_LIMIT = 1200.f;

    auto& fields = *m_fields.self();
    auto& am = AudioManager::get();

    if (am.getDeafen() || !fields.m_isVoiceProximity) {
        return globed::setting<float>("core.audio.playback-volume");
    }

    if (!fields.m_interpolator.hasPlayer(playerId)) {
        return 0.f;
    }

    OutFlags flags;
    auto& vstate = fields.m_interpolator.getPlayerState(playerId, flags);

    float distance = ccpDistance(m_player1->getPosition(), vstate.player1->position);
    float volume = 1.f - std::clamp(distance, 0.01f, PROXIMITY_VOICE_LIMIT) / PROXIMITY_VOICE_LIMIT;
    if (vstate.isInEditor) {
        volume = 1.f;
    }

    volume *= globed::setting<float>("core.audio.playback-volume");
    return volume;
}

}
