#include "play_layer.hpp"

#if GLOBED_HAS_KEYBINDS
# include <geode.custom-keybinds/include/Keybinds.hpp>
#endif // GLOBED_HAS_KEYBINDS

#include "pause_layer.hpp"
#include <audio/all.hpp>
#include <managers/block_list.hpp>
#include <managers/error_queues.hpp>
#include <managers/friend_list.hpp>
#include <managers/profile_cache.hpp>
#include <managers/settings.hpp>
#include <data/packets/all.hpp>
#include <util/math.hpp>
#include <util/debug.hpp>

using namespace geode::prelude;

// how many units before the voice disappears
constexpr float PROXIMITY_VOICE_LIMIT = 1200.f;

constexpr float VOICE_OVERLAY_PAD_X = 5.f;
constexpr float VOICE_OVERLAY_PAD_Y = 20.f;

float adjustLerpTimeDelta(float dt) {
    // i fucking hate this i cannot do this anymore i want to die
    auto* dir = CCDirector::get();
    // TODO idk if timescale is needed here i will need to test with speedhack
    return dir->getAnimationInterval(); // * CCScheduler::get()->getTimeScale();

    // uncomment and watch the world blow up
    // return dt;
}

bool GlobedPlayLayer::init(GJGameLevel* level, bool p1, bool p2) {
    if (!PlayLayer::init(level, p1, p2)) return false;

    GlobedSettings& settings = GlobedSettings::get();

    auto winSize = CCDirector::get()->getWinSize();
    float overlayBaseY = settings.overlay.position < 2 ? winSize.height - 2.f : 2.f;
    float overlayBaseX = (settings.overlay.position % 2 == 1) ? winSize.width - 2.f : 2.f;

    float overlayAnchorY = (settings.overlay.position < 2) ? 1.f : 0.f;
    float overlayAnchorX = (settings.overlay.position % 2 == 1) ? 1.f : 0.f;

    // overlay
    Build<GlobedOverlay>::create()
        .pos(overlayBaseX, overlayBaseY)
        .anchorPoint(overlayAnchorX, overlayAnchorY)
        .scale(0.4f)
        .zOrder(11)
        .id("game-overlay"_spr)
        .parent(this)
        .store(m_fields->overlay);

    auto& nm = NetworkManager::get();

    // if not authenticated, do nothing
    bool isEditor = m_level->m_levelType == GJLevelType::Editor;
    m_fields->globedReady = nm.established() && !isEditor;

    if (!nm.established()) {
        m_fields->overlay->updateWithDisconnected();
        return true;
    } else if (isEditor) {
        m_fields->overlay->updateWithEditor();
        return true;
    }

    m_fields->isVoiceProximity = level->isPlatformer() ? settings.communication.voiceProximity : settings.communication.classicProximity;

    // else update the overlay with ping
    m_fields->overlay->updatePing(GameServerManager::get().getActivePing());

    // set the configured tps
    auto tpsCap = settings.globed.tpsCap;
    if (tpsCap != 0) {
        m_fields->configuredTps = std::min(nm.connectedTps.load(), (uint32_t)tpsCap);
    } else {
        m_fields->configuredTps = nm.connectedTps;
    }

#if GLOBED_VOICE_SUPPORT
    // set the audio device
    auto& vm = GlobedAudioManager::get();
    try {
        vm.setActiveRecordingDevice(settings.communication.audioDevice);
    } catch (const std::exception& e) {
        // try default device, if we have no mic then just do nothing
        settings.save();
        try {
            vm.setActiveRecordingDevice(0);
        } catch (const std::exception& _e) {
            settings.communication.audioDevice = -1;
        }

        settings.communication.audioDevice = 0;
    }

    // set the record buffer size
    vm.setRecordBufferCapacity(settings.communication.lowerAudioLatency ? EncodedAudioFrame::LIMIT_LOW_LATENCY : EncodedAudioFrame::LIMIT_REGULAR);

    // start passive voice recording
    // TODO temp remove on android because it crashes the game :fire:
    if (settings.communication.voiceEnabled) {
# ifndef GEODE_IS_ANDROID
        auto& vrm = VoiceRecordingManager::get();
        vrm.startRecording();
# endif // GEODE_IS_ANDROID

        if (settings.levelUi.voiceOverlay) {
            m_fields->voiceOverlay = Build<GlobedVoiceOverlay>::create()
                .parent(m_uiLayer)
                .pos(winSize.width - VOICE_OVERLAY_PAD_X, VOICE_OVERLAY_PAD_Y)
                .anchorPoint(1.f, 0.f)
                .collect();
        }
    }

#endif // GLOBED_VOICE_SUPPORT

    // send SyncIconsPacket if our icons have changed since the last time we sent it
    auto& pcm = ProfileCacheManager::get();
    pcm.setOwnDataAuto();
    if (pcm.pendingChanges) {
        pcm.pendingChanges = false;
        nm.send(SyncIconsPacket::create(pcm.getOwnData()));
    }

    this->setupCustomKeybinds();

    // interpolator
    m_fields->interpolator = std::make_shared<PlayerInterpolator>(InterpolatorSettings {
        .realtime = false,
        .isPlatformer = m_level->isPlatformer(),
        .expectedDelta = (1.0f / m_fields->configuredTps)
    });

    // player store
    m_fields->playerStore = std::make_shared<PlayerStore>();

    // update
    Loader::get()->queueInMainThread([&nm] {
        auto self = static_cast<GlobedPlayLayer*>(PlayLayer::get());
        if (!self) return;

        // here we run the stuff that must run on a valid playlayer
        self->setupPacketListeners();

        // send LevelJoinPacket and RequestPlayerProfilesPacket
        nm.send(LevelJoinPacket::create(self->m_level->m_levelID));
        nm.send(RequestPlayerProfilesPacket::create(0));

        self->rescheduleSelectors();
        CCScheduler::get()->scheduleSelector(schedule_selector(GlobedPlayLayer::selUpdate), self->getParent(), 0.0f, false);
    });

    m_fields->progressBarWrapper = Build<CCNode>::create()
        .id("progress-bar-wrapper"_spr)
        .visible(settings.levelUi.progressIndicators)
        .zOrder(-1)
        .collect();

    m_fields->selfProgressIcon = Build<PlayerProgressIcon>::create()
        .parent(m_fields->progressBarWrapper)
        .id("self-player-progress"_spr)
        .collect();

    m_fields->selfProgressIcon->updateIcons(ProfileCacheManager::get().getOwnData());
    m_fields->selfProgressIcon->setForceOnTop(true);

    if (settings.players.statusIcons) {
        m_fields->selfStatusIcons = Build<PlayerStatusIcons>::create()
            .scale(0.8f)
            .anchorPoint(0.5f, 0.f)
            .pos(0.f, 25.f)
            .parent(m_objectLayer)
            .id("self-status-icon"_spr)
            .collect();
    }

    auto& flm = FriendListManager::get();
    flm.maybeLoad();

    return true;
}

void GlobedPlayLayer::onQuit() {
    PlayLayer::onQuit();

    if (!m_fields->globedReady) return;

    auto& nm = NetworkManager::get();
    if (!nm.established()) return;

    // send LevelLeavePacket
    nm.send(LevelLeavePacket::create());

#if GLOBED_VOICE_SUPPORT
    // stop voice recording and playback
    GlobedAudioManager::get().haltRecording();
    VoicePlaybackManager::get().stopAllStreams();
#endif // GLOBED_VOICE_SUPPORT

    // clean up the listeners
    nm.removeListener<PlayerProfilesPacket>(util::time::seconds(3));
    nm.removeListener<LevelDataPacket>(util::time::seconds(3));
    nm.removeListener<VoiceBroadcastPacket>(util::time::seconds(3));
}

void GlobedPlayLayer::setupPacketListeners() {
    auto& nm = NetworkManager::get();

    nm.addListener<PlayerProfilesPacket>([](std::shared_ptr<PlayerProfilesPacket> packet) {
        auto& pcm = ProfileCacheManager::get();
        for (auto& player : packet->players) {
            pcm.insert(player);
        }
    });

    nm.addListener<LevelDataPacket>([this](std::shared_ptr<LevelDataPacket> packet){
        this->m_fields->lastServerUpdate = this->m_fields->timeCounter;

        for (const auto& player : packet->players) {
            if (!this->m_fields->players.contains(player.accountId)) {
                // new player joined
                this->handlePlayerJoin(player.accountId);
            }

            this->m_fields->playerStore->insertOrUpdate(player.accountId, player.data.attempts, player.data.localBest);
            this->m_fields->interpolator->updatePlayer(player.accountId, player.data, this->m_fields->lastServerUpdate);
        }
    });

    nm.addListener<VoiceBroadcastPacket>([this](std::shared_ptr<VoiceBroadcastPacket> packet) {
#if GLOBED_VOICE_SUPPORT
        // if deafened or voice is disabled, do nothing
        auto& settings = GlobedSettings::get();

        if (this->m_fields->deafened || !settings.communication.voiceEnabled) return;
        if (!this->shouldLetMessageThrough(packet->sender)) return;

        auto& vpm = VoicePlaybackManager::get();
        try {
            vpm.prepareStream(packet->sender);

            vpm.setVolume(packet->sender, settings.communication.voiceVolume);
            this->updateProximityVolume(packet->sender);
            vpm.playFrameStreamed(packet->sender, packet->frame);
        } catch(const std::exception& e) {
            ErrorQueues::get().debugWarn(std::string("Failed to play a voice frame: ") + e.what());
        }
#endif // GLOBED_VOICE_SUPPORT
    });
}

void GlobedPlayLayer::setupCustomKeybinds() {
#if GLOBED_HAS_KEYBINDS && GLOBED_VOICE_SUPPORT
    // the only keybinds used are for voice chat, so if voice is disabled, do nothing

    auto& settings = GlobedSettings::get();
    if (!settings.communication.voiceEnabled) {
        return;
    }

    this->addEventListener<keybinds::InvokeBindFilter>([this](keybinds::InvokeBindEvent* event) {
        if (event->isDown()) {
            if (!this->m_fields->deafened) {
                GlobedAudioManager::get().resumePassiveRecording();
            }
        } else {
            GlobedAudioManager::get().pausePassiveRecording();
        }

        return ListenerResult::Stop;
    }, "voice-activate"_spr);

    this->addEventListener<keybinds::InvokeBindFilter>([this, &settings](keybinds::InvokeBindEvent* event) {
        auto& vpm = VoicePlaybackManager::get();

        if (event->isDown()) {
            this->m_fields->deafened = !this->m_fields->deafened;
            if (this->m_fields->deafened) {
                vpm.muteEveryone();
                GlobedAudioManager::get().pausePassiveRecording();
            } else if (!this->m_fields->isVoiceProximity) {
                vpm.setVolumeAll(settings.communication.voiceVolume);
            }
        }

        return ListenerResult::Propagate;
    }, "voice-deafen"_spr);
#endif // GLOBED_HAS_KEYBINDS && GLOBED_VOICE_SUPPORT
}

// selSendPlayerData - runs tps (default 30) times per second
void GlobedPlayLayer::selSendPlayerData(float) {
    auto self = static_cast<GlobedPlayLayer*>(PlayLayer::get());

    if (!self || !self->established()) return;
    if (!self->isCurrentPlayLayer()) return;
    if (!self->accountForSpeedhack(0, 1.0f / self->m_fields->configuredTps, 0.8f)) return;

    self->m_fields->totalSentPackets++;
    // additionally, if there are no players on the level, we drop down to 1 time per second as an optimization
    if (self->m_fields->players.empty() && self->m_fields->totalSentPackets % 30 != 15) return;

    auto data = self->gatherPlayerData();
    NetworkManager::get().send(PlayerDataPacket::create(data));
}

// selPeriodicalUpdate - runs 4 times a second, does various stuff
void GlobedPlayLayer::selPeriodicalUpdate(float) {
    auto self = static_cast<GlobedPlayLayer*>(PlayLayer::get());

    if (!self || !self->established()) return;
    if (!self->isCurrentPlayLayer()) return;

    // update the overlay
    self->m_fields->overlay->updatePing(GameServerManager::get().getActivePing());

    auto& pcm = ProfileCacheManager::get();

    util::collections::SmallVector<int, 32> toRemove;

    // if more than a second passed and there was only 1 player, they probably left
    if (self->m_fields->timeCounter - self->m_fields->lastServerUpdate > 1.0f && self->m_fields->players.size() < 2) {
        for (const auto& [playerId, _] : self->m_fields->players) {
            toRemove.push_back(playerId);
        }

        for (int id : toRemove) {
            self->handlePlayerLeave(id);
        }

        return;
    }

    util::collections::SmallVector<int, 256> ids;

    // kick players that have left the level
    for (const auto& [playerId, remotePlayer] : self->m_fields->players) {
        // if the player doesnt exist in last LevelData packet, they have left the level
        if (self->m_fields->interpolator->isPlayerStale(playerId, self->m_fields->lastServerUpdate)) {
            toRemove.push_back(playerId);
            continue;
        }

        auto data = pcm.getData(playerId);

        if (!remotePlayer->isValidPlayer()) {
            if (data.has_value()) {
                // if the profile data already exists in cache, use it
                remotePlayer->updateAccountData(data.value(), true);
            } else if (remotePlayer->getDefaultTicks() >= 12) {
                // if it has been 3 seconds and we still don't have them in cache, request again
                remotePlayer->setDefaultTicks(0);
                ids.push_back(playerId);
            } else {
                // if it has been less than 3 seconds, just increment the tick counter
                remotePlayer->incDefaultTicks();
            }
        } else if (data.has_value()) {
            // still try to see if the cache has changed
            remotePlayer->updateAccountData(data.value());
        }
    }

    if (ids.size() > 3) {
        NetworkManager::get().send(RequestPlayerProfilesPacket::create(0));
    } else {
        for (int id : ids) {
            NetworkManager::get().send(RequestPlayerProfilesPacket::create(id));
        }
    }

    for (int id : toRemove) {
        self->handlePlayerLeave(id);
    }
}

// selUpdate - runs every frame, increments the non-decreasing time counter, interpolates and updates players
void GlobedPlayLayer::selUpdate(float rawdt) {
    auto self = static_cast<GlobedPlayLayer*>(PlayLayer::get());

    if (!self) return;

    auto visibleOrigin = CCPoint{0.f, 0.f};
    auto visibleCoverage = CCDirector::get()->getWinSize();

    auto camOrigin = self->m_gameState.m_unk20c; // m_unk20c or m_unk2c0, same values
    auto zoom = self->m_objectLayer->getScale();
    auto camCoverage = visibleCoverage / zoom;

    // update ourselves
    auto accountId = GJAccountManager::get()->m_accountID;
    self->m_fields->playerStore->insertOrUpdate(
        accountId,
        self->m_level->m_attempts,
        static_cast<uint32_t>(self->m_level->isPlatformer() ? self->m_level->m_bestTime : self->m_level->m_normalPercent)
    );

    float dt = adjustLerpTimeDelta(rawdt);
    self->m_fields->timeCounter += dt;

    self->m_fields->interpolator->tick(dt);

    if (self->m_fields->progressBarWrapper->getParent() != nullptr) {
        self->m_fields->selfProgressIcon->updatePosition(self->getCurrentPercent() / 100.f);
    } else if (self->m_progressBar) {
        // for some reason, the progressbar is sometimes initialized later than PlayLayer::init
        // it always should exist, even in levels with no actual progress bar (i.e. platformer levels)
        // but it can randomly get initialized late.
        // why robtop????????
        self->m_progressBar->addChild(self->m_fields->progressBarWrapper);
    }

    auto& bl = BlockListManager::get();
    auto& vpm = VoicePlaybackManager::get();
    auto& settings = GlobedSettings::get();

    for (const auto [playerId, remotePlayer] : self->m_fields->players) {
        const auto& vstate = self->m_fields->interpolator->getPlayerState(playerId);

        bool playDeathEffect = self->m_fields->interpolator->swapDeathStatus(playerId);
        auto p1tp = self->m_fields->interpolator->swapP1Teleport(playerId);
        auto p2tp = self->m_fields->interpolator->swapP2Teleport(playerId);

        bool isSpeaking = vpm.isSpeaking(playerId);
        remotePlayer->updateData(vstate, playDeathEffect, isSpeaking, p1tp, p2tp, isSpeaking ? vpm.getLoudness(playerId) : 0.f);

        if (self->m_progressBar && self->m_progressBar->isVisible() && remotePlayer->progressIcon) {
            remotePlayer->updateProgressIcon();
        } else if (remotePlayer->progressArrow) {
            remotePlayer->updateProgressArrow(camOrigin, camCoverage, visibleOrigin, visibleCoverage, zoom);
        }

        // update voice proximity
        self->updateProximityVolume(playerId);
    }

    if (self->m_fields->selfStatusIcons) {
        self->m_fields->selfStatusIcons->setPosition(self->m_player1->getPosition() + CCPoint{0.f, 25.f});
        bool recording = VoiceRecordingManager::get().isRecording();

        self->m_fields->selfStatusIcons->updateStatus(false, false, recording, 0.f);
    }

    if (self->m_fields->voiceOverlay) {
        self->m_fields->voiceOverlay->updateOverlaySoft();
    }
}

// selUpdateEstimators - runs 30 times a second, updates audio stuff
void GlobedPlayLayer::selUpdateEstimators(float dt) {
    auto* self = static_cast<GlobedPlayLayer*>(PlayLayer::get());

    // update volume estimators
    VoicePlaybackManager::get().updateAllEstimators(dt);

    if (self->m_fields->voiceOverlay) {
        self->m_fields->voiceOverlay->updateOverlay();
    }
}

/* private utilities */

bool GlobedPlayLayer::shouldLetMessageThrough(int playerId) {
    auto& settings = GlobedSettings::get();
    auto& flm = FriendListManager::get();
    auto& bl = BlockListManager::get();

    if (bl.isExplicitlyBlocked(playerId)) return false;
    if (bl.isExplicitlyAllowed(playerId)) return true;

    if (settings.communication.onlyFriends && !flm.isFriend(playerId)) return false;

    return true;
}

void GlobedPlayLayer::updateProximityVolume(int playerId) {
    if (m_fields->deafened || !m_fields->isVoiceProximity) return;

    auto& vpm = VoicePlaybackManager::get();

    if (!m_fields->interpolator->hasPlayer(playerId)) {
        // if we have no knowledge on the player, set volume to 0
        vpm.setVolume(playerId, 0.f);
        return;
    }

    auto& vstate = m_fields->interpolator->getPlayerState(playerId);

    float distance = cocos2d::ccpDistance(m_player1->getPosition(), vstate.player1.position);
    float volume = 1.f - std::clamp(distance, 0.01f, PROXIMITY_VOICE_LIMIT) / PROXIMITY_VOICE_LIMIT;

    auto& settings = GlobedSettings::get();

    if (this->shouldLetMessageThrough(playerId)) {
        vpm.setVolume(playerId, volume * settings.communication.voiceVolume);
    }
}

SpecificIconData GlobedPlayLayer::gatherSpecificIconData(PlayerObject* player) {
    PlayerIconType iconType = PlayerIconType::Cube;
    if (player->m_isShip) iconType = PlayerIconType::Ship;
    else if (player->m_isBird) iconType = PlayerIconType::Ufo;
    else if (player->m_isBall) iconType = PlayerIconType::Ball;
    else if (player->m_isDart) iconType = PlayerIconType::Wave;
    else if (player->m_isRobot) iconType = PlayerIconType::Robot;
    else if (player->m_isSpider) iconType = PlayerIconType::Spider;
    else if (player->m_isSwing) iconType = PlayerIconType::Swing;

    auto* pobjInner = static_cast<CCNode*>(player->getChildren()->objectAtIndex(0));

    float rot = player->getRotation() + pobjInner->getRotation();

    bool isPlayer1 = player == m_player1;

    auto spiderTeleportData = isPlayer1 ? util::misc::swapOptional(m_fields->spiderTp1) : util::misc::swapOptional(m_fields->spiderTp2);

    bool isStationary = false;
    if (m_level->isPlatformer()) {
        isStationary = std::abs(player->m_platformerXVelocity) < 0.1;
    }

    return SpecificIconData {
        .position = player->getPosition(),
        .rotation = rot,

        .iconType = iconType,
        .isVisible = isPlayer1 ? player->isVisible() : (m_gameState.m_isDualMode && player->isVisible()),
        .isLookingLeft = player->m_isGoingLeft,
        .isUpsideDown = player->m_isSwing ? player->m_isUpsideDown : pobjInner->getScaleY() == -1.0f,
        .isDashing = player->m_isDashing,
        .isMini = player->m_vehicleSize != 1.0f,
        .isGrounded = player->m_isOnGround,
        .isStationary = isStationary,
        .isFalling = player->m_yVelocity < 0.0,
        .spiderTeleportData = spiderTeleportData
    };
}

PlayerData GlobedPlayLayer::gatherPlayerData() {
    bool isDead = m_player1->m_isDead || m_player2->m_isDead;
    if (isDead && !m_fields->isCurrentlyDead) {
        m_fields->isCurrentlyDead = true;
        m_fields->lastDeathTimestamp = m_fields->timeCounter;
    } else if (!isDead) {
        m_fields->isCurrentlyDead = false;
    }

    uint32_t localBest;
    if (m_level->isPlatformer()) {
        localBest = static_cast<uint32_t>(m_level->m_bestTime);
    } else {
        localBest = static_cast<uint32_t>(m_level->m_normalPercent);
    }

    return PlayerData {
        .timestamp = m_fields->timeCounter,
        .localBest = localBest,
        .attempts = m_level->m_attempts,

        .player1 = this->gatherSpecificIconData(m_player1),
        .player2 = this->gatherSpecificIconData(m_player2), // TODO detect isDualMode

        .lastDeathTimestamp = m_fields->lastDeathTimestamp,

        .currentPercentage = this->getCurrentPercent() / 100.f,

        .isDead = isDead,
        .isPaused = this->isPaused(),
        .isPracticing = m_isPracticeMode,
    };
}

void GlobedPlayLayer::handlePlayerJoin(int playerId) {
    auto& settings = GlobedSettings::get();

    PlayerProgressIcon* progressIcon = nullptr;
    PlayerProgressArrow* progressArrow = nullptr;

    bool platformer = m_level->isPlatformer();

    if (!platformer && settings.levelUi.progressIndicators) {
        Build<PlayerProgressIcon>::create()
            .zOrder(2)
            .id(Mod::get()->expandSpriteName(fmt::format("remote-player-progress-{}", playerId).c_str()))
            .parent(m_fields->progressBarWrapper)
            .store(progressIcon);
    } else if (platformer && settings.levelUi.progressIndicators) {
        Build<PlayerProgressArrow>::create()
            .zOrder(2)
            .id(Mod::get()->expandSpriteName(fmt::format("remote-player-progress-{}", playerId).c_str()))
            .parent(this->getParent())
            .store(progressArrow);
    }

    auto* rp = Build<RemotePlayer>::create(progressIcon, progressArrow)
        .zOrder(10) // TODO temp
        .id(Mod::get()->expandSpriteName(fmt::format("remote-player-{}", playerId).c_str()))
        .collect();

    auto& pcm = ProfileCacheManager::get();
    auto pcmData = pcm.getData(playerId);
    if (pcmData.has_value()) {
        rp->updateAccountData(pcmData.value(), true);
    }

    m_objectLayer->addChild(rp);
    m_fields->players.emplace(playerId, rp);
    m_fields->interpolator->addPlayer(playerId);

    // log::debug("Player joined: {}", playerId);
}

void GlobedPlayLayer::handlePlayerLeave(int playerId) {
    VoicePlaybackManager::get().removeStream(playerId);

    if (!m_fields->players.contains(playerId)) return;

    auto rp = m_fields->players.at(playerId);
    rp->removeProgressIndicators();
    rp->removeFromParent();

    m_fields->players.erase(playerId);
    m_fields->interpolator->removePlayer(playerId);
    m_fields->playerStore->removePlayer(playerId);

    // log::debug("Player removed: {}", playerId);
}

bool GlobedPlayLayer::accountForSpeedhack(size_t uniqueKey, float cap, float allowance) { // TODO test if this still works for classic speedhax
    auto* sched = CCScheduler::get();
    auto ts = sched->getTimeScale();
    if (!util::math::equal(ts, m_fields->lastKnownTimeScale)) {
        this->unscheduleSelectors();
        this->rescheduleSelectors();
    }

    auto now = util::time::now();

    if (!m_fields->lastSentPacket.contains(uniqueKey)) {
        m_fields->lastSentPacket.emplace(uniqueKey, now);
        return true;
    }

    auto lastSent = m_fields->lastSentPacket.at(uniqueKey);

    auto passed = util::time::asMillis(now - lastSent);

    if (passed < cap * 1000 * allowance) {
        // log::warn("dropping a packet (speedhack?), passed time is {}ms", passed);
        return false;
    }

    m_fields->lastSentPacket[uniqueKey] = now;

    return true;
}

void GlobedPlayLayer::unscheduleSelectors() {
    auto* sched = CCScheduler::get();
    sched->unscheduleSelector(schedule_selector(GlobedPlayLayer::selSendPlayerData), this->getParent());
    sched->unscheduleSelector(schedule_selector(GlobedPlayLayer::selPeriodicalUpdate), this->getParent());
    sched->unscheduleSelector(schedule_selector(GlobedPlayLayer::selUpdateEstimators), this->getParent());
}

void GlobedPlayLayer::rescheduleSelectors() {
    auto* sched = CCScheduler::get();
    float timescale = sched->getTimeScale();
    m_fields->lastKnownTimeScale = timescale;

    float pdInterval = (1.0f / m_fields->configuredTps) * timescale;
    float updpInterval = 0.25f * timescale;
    float updeInterval = (1.0f / 30.f) * timescale;

    sched->scheduleSelector(schedule_selector(GlobedPlayLayer::selSendPlayerData), this->getParent(), pdInterval, false);
    sched->scheduleSelector(schedule_selector(GlobedPlayLayer::selPeriodicalUpdate), this->getParent(), updpInterval, false);
    sched->scheduleSelector(schedule_selector(GlobedPlayLayer::selUpdateEstimators), this->getParent(), updeInterval, false);
}
