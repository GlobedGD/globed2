#include "play_layer.hpp"

#if GLOBED_HAS_KEYBINDS
# include <geode.custom-keybinds/include/Keybinds.hpp>
#endif // GLOBED_HAS_KEYBINDS

#include "pause_layer.hpp"
#include <audio/all.hpp>
#include <managers/profile_cache.hpp>
#include <managers/error_queues.hpp>
#include <managers/settings.hpp>
#include <data/packets/all.hpp>
#include <util/math.hpp>
#include <util/debugging.hpp>

using namespace geode::prelude;

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

    auto winSize = CCDirector::get()->getWinSize();

    // overlay
    Build<GlobedOverlay>::create()
        .pos(2.f, winSize.height - 2.f) // TODO configurable pos
        .anchorPoint(0.f, 1.f)
        .scale(0.4f)
        .zOrder(11)
        .id("game-overlay"_spr)
        .parent(this)
        .store(m_fields->overlay);

    auto& nm = NetworkManager::get();

    // if not authenticated, do nothing
    m_fields->globedReady = nm.established();
    if (!m_fields->globedReady) {
        m_fields->overlay->updateWithDisconnected(); // TODO in an editor level do updateWithEditor
        return true;
    }

    // else update the overlay with ping
    m_fields->overlay->updatePing(GameServerManager::get().getActivePing());

    GlobedSettings& settings = GlobedSettings::get();

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
    } catch(const std::exception& e) {
        // try default device, if we have no mic then just do nothing
        try {
            vm.setActiveRecordingDevice(0);
        } catch (const std::exception& _e) {}
    }

    // set the record buffer size
    vm.setRecordBufferCapacity(settings.communication.lowerAudioLatency ? EncodedAudioFrame::LIMIT_LOW_LATENCY : EncodedAudioFrame::LIMIT_REGULAR);
#endif // GLOBED_VOICE_SUPPORT

    // send SyncIconsPacket if our icons have changed since the last time we sent it
    auto& pcm = ProfileCacheManager::get();
    pcm.setOwnDataAuto();
    if (pcm.pendingChanges) {
        pcm.pendingChanges = false;
        nm.send(SyncIconsPacket::create(pcm.getOwnData()));
    }

    // send LevelJoinPacket and RequestPlayerProfilesPacket
    nm.send(LevelJoinPacket::create(level->m_levelID));
    nm.send(RequestPlayerProfilesPacket::create(0));

    this->setupPacketListeners();
    this->setupCustomKeybinds();

    // interpolator
    m_fields->interpolator = std::make_shared<PlayerInterpolator>(InterpolatorSettings {
        .realtime = false,
        .isPlatformer = m_level->isPlatformer(),
        .expectedDelta = (1.0f / m_fields->configuredTps)
    });

    // update
    Loader::get()->queueInMainThread([this] {
        this->rescheduleSelectors();
        CCScheduler::get()->scheduleSelector(schedule_selector(GlobedPlayLayer::selUpdate), this->getParent(), 0.0f, false);
    });

    m_fields->selfProgressIcon = Build<PlayerProgressIcon>::create(m_levelLength)
        .parent(m_progressBar)
        .id("self-player-progress"_spr)
        .collect();

    return true;
}

void GlobedPlayLayer::onQuit() {
    PlayLayer::onQuit();

    if (!m_fields->globedReady) return;

#if GLOBED_VOICE_SUPPORT
    // stop voice recording and playback
    GlobedAudioManager::get().haltRecording();
    VoicePlaybackManager::get().stopAllStreams();
#endif // GLOBED_VOICE_SUPPORT

    auto& nm = NetworkManager::get();

    // clean up the listeners
    nm.removeListener<PlayerProfilesPacket>();
    nm.removeListener<LevelDataPacket>();
    nm.removeListener<VoiceBroadcastPacket>();

    if (!nm.established()) return;

    // send LevelLeavePacket
    nm.send(LevelLeavePacket::create());

    // if we a have a higher ping, there may still be some inbound packets. suppress them for the next second.
    nm.suppressUnhandledFor<PlayerProfilesPacket>(util::time::secs(1));
    nm.suppressUnhandledFor<LevelDataPacket>(util::time::secs(1));
    nm.suppressUnhandledFor<VoiceBroadcastPacket>(util::time::secs(1));
}

void GlobedPlayLayer::setupPacketListeners() {
    auto& nm = NetworkManager::get();

    nm.addListener<PlayerProfilesPacket>([](PlayerProfilesPacket* packet) {
        auto& pcm = ProfileCacheManager::get();
        for (auto& player : packet->players) {
            pcm.insert(player);
        }
    });

    nm.addListener<LevelDataPacket>([this](LevelDataPacket* packet){
        this->m_fields->lastServerUpdate = this->m_fields->timeCounter;

        for (const auto& player : packet->players) {
            if (!this->m_fields->players.contains(player.accountId)) {
                // new player joined
                this->handlePlayerJoin(player.accountId);
            }

            this->m_fields->interpolator->updatePlayer(player.accountId, player.data, this->m_fields->lastServerUpdate);
        }
    });

    nm.addListener<VoiceBroadcastPacket>([this](VoiceBroadcastPacket* packet) {
#if GLOBED_VOICE_SUPPORT
        // if deafened or voice is disabled, do nothing
        auto& settings = GlobedSettings::get();
        if (this->m_fields->deafened || !settings.communication.voiceEnabled) return;

        // TODO client side blocking and stuff..
        log::debug("streaming frame from {}", packet->sender);
        // TODO - this decodes the sound data on the main thread. might be a bad idea, will need to benchmark.
        try {
            VoicePlaybackManager::get().playFrameStreamed(packet->sender, packet->frame);
        } catch(const std::exception& e) {
            ErrorQueues::get().debugWarn(std::string("Failed to play a voice frame: ") + e.what());
        }
#endif // GLOBED_VOICE_SUPPORT
    });
}

void GlobedPlayLayer::setupCustomKeybinds() {
#if GLOBED_HAS_KEYBINDS && GLOBED_VOICE_SUPPORT
    // the only keybinds used are for voice chat, so if voice is disabled, do nothing
    if (!GlobedSettings::get().communication.voiceEnabled) {
        return;
    }

    this->addEventListener<keybinds::InvokeBindFilter>([this](keybinds::InvokeBindEvent* event) {
        auto& vm = GlobedAudioManager::get();

        if (event->isDown()) {
            if (!this->m_fields->deafened && !vm.isRecording()) {
                // make sure the recording device is valid
                vm.validateDevices();
                if (!vm.isRecordingDeviceSet()) {
                    ErrorQueues::get().warn("Unable to record audio, no recording device is set");
                    return ListenerResult::Stop;
                }

                auto result = vm.startRecording([](const auto& frame) {
                    // (!) remember that the callback is ran from another thread (!) //

                    auto& nm = NetworkManager::get();
                    if (!nm.established()) return;

                    // `frame` does not live long enough and will be destructed at the end of this callback.
                    // so we can't pass it directly in a `VoicePacket` and we use a `RawPacket` instead.

                    ByteBuffer buf;
                    buf.writeValue(frame);

                    nm.send(RawPacket::create(VoicePacket::PACKET_ID, VoicePacket::ENCRYPTED, std::move(buf)));
                });

                if (result.isErr()) {
                    ErrorQueues::get().warn(result.unwrapErr());
                    geode::log::warn("unable to record audio: {}", result.unwrapErr());
                }
            }
        } else {
            if (vm.isRecording()) {
                vm.stopRecording();
            }
        }

        return ListenerResult::Stop;
    }, "voice-activate"_spr);

    this->addEventListener<keybinds::InvokeBindFilter>([this](keybinds::InvokeBindEvent* event) {
        if (event->isDown()) {
            this->m_fields->deafened = !this->m_fields->deafened;
        }

        return ListenerResult::Propagate;
    }, "voice-deafen"_spr);
#endif // GLOBED_HAS_KEYBINDS && GLOBED_VOICE_SUPPORT
}

// selSendPlayerData - runs tps (default 30) times per second
void GlobedPlayLayer::selSendPlayerData(float) {
    auto self = static_cast<GlobedPlayLayer*>(PlayLayer::get());

    if (!self->established()) return;
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

    if (!self->established()) return;
    if (!self->isCurrentPlayLayer()) return;

    // update the overlay
    self->m_fields->overlay->updatePing(GameServerManager::get().getActivePing());

    auto& pcm = ProfileCacheManager::get();

    util::collections::SmallVector<int, 16> toRemove;

    // if there has been no server update for a while, likely there are no players on the level, kick everyone
    if (self->m_fields->timeCounter - self->m_fields->lastServerUpdate > 1.0f) {
        for (const auto& [playerId, _] : self->m_fields->players) {
            toRemove.push_back(playerId);
        }

        for (int id : toRemove) {
            self->handlePlayerLeave(id);
        }

        return;
    }

    // kick players that have left the level
    for (const auto& [playerId, remotePlayer] : self->m_fields->players) {
        // if the player doesnt exist in last LevelData packet, they have left the level
        if (self->m_fields->interpolator->isPlayerStale(playerId, self->m_fields->lastServerUpdate)) {
            toRemove.push_back(playerId);
            continue;
        }

        if (!remotePlayer->isValidPlayer()) {
            auto data = pcm.getData(playerId);
            if (data.has_value()) {
                // if the profile data already exists in cache, use it
                remotePlayer->updateAccountData(data.value());
            } else if (remotePlayer->getDefaultTicks() >= 12) {
                // if it has been 3 seconds and we still don't have them in cache, request again
                remotePlayer->setDefaultTicks(0);
                NetworkManager::get().send(RequestPlayerProfilesPacket::create(playerId));
            } else {
                // if it has been less than 3 seconds, just increment the tick counter
                remotePlayer->incDefaultTicks();
            }
        }
    }

    for (int id : toRemove) {
        self->handlePlayerLeave(id);
    }
}

// selUpdate - runs every frame, increments the non-decreasing time counter, interpolates and updates players
void GlobedPlayLayer::selUpdate(float rawdt) {
    auto self = static_cast<GlobedPlayLayer*>(PlayLayer::get());

    float dt = adjustLerpTimeDelta(rawdt);
    self->m_fields->timeCounter += dt;

    self->m_fields->interpolator->tick(dt);

    for (const auto [playerId, remotePlayer] : self->m_fields->players) {
        const auto& vstate = self->m_fields->interpolator->getPlayerState(playerId);
        remotePlayer->updateData(vstate, self->m_fields->interpolator->swapDeathStatus(playerId));

        if (self->m_progressBar->isVisible()) {
            remotePlayer->updateProgressIcon();
        }
    }

    self->m_fields->selfProgressIcon->updatePosition(self->m_player1->m_position.x); // idk maybe use getPosition().x
}

/* private utilities */

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

    return SpecificIconData {
        .position = player->getPosition(), // TODO maybe use m_position
        .rotation = rot,

        .iconType = iconType,
        .isVisible = player->isVisible(),
        .isLookingLeft = player->m_isGoingLeft,
        .isUpsideDown = pobjInner->getScaleY() == -1.0f,
        .isDashing = player->m_isDashing,
        .isMini = player->m_vehicleSize != 1.0f
    };
}

PlayerData GlobedPlayLayer::gatherPlayerData() {
    bool isDead = false; // TODO
    if (isDead && !m_fields->isCurrentlyDead) {
        m_fields->isCurrentlyDead = true;
        m_fields->lastDeathTimestamp = m_fields->timeCounter;
    } else if (!isDead) {
        m_fields->isCurrentlyDead = false;
    }

    return PlayerData {
        .timestamp = m_fields->timeCounter,
        .percentage = static_cast<uint16_t>(m_level->m_normalPercent),
        .attempts = m_level->m_attempts,

        .player1 = this->gatherSpecificIconData(m_player1),
        .player2 = this->gatherSpecificIconData(m_player2), // TODO detect isDualMode

        .lastDeathTimestamp = m_fields->lastDeathTimestamp,

        .isDead = isDead,
        .isPaused = this->isPaused(),
        .isPracticing = m_isPracticeMode
    };
}

void GlobedPlayLayer::handlePlayerJoin(int playerId) {
#if GLOBED_VOICE_SUPPORT
    try {
        VoicePlaybackManager::get().prepareStream(playerId);
    } catch (const std::exception& e) {
        ErrorQueues::get().error(std::string("Failed to prepare audio stream: ") + e.what());
    }
#endif // GLOBED_VOICE_SUPPORT
    NetworkManager::get().send(RequestPlayerProfilesPacket::create(playerId));

    auto* progressIcon = Build<PlayerProgressIcon>::create(m_levelLength)
        .zOrder(2)
        .id(Mod::get()->expandSpriteName(fmt::format("remote-player-progress-{}", playerId).c_str()))
        .parent(m_progressBar)
        .collect();

    auto* rp = Build<RemotePlayer>::create(progressIcon)
        .zOrder(10) // TODO temp
        .id(Mod::get()->expandSpriteName(fmt::format("remote-player-{}", playerId).c_str()))
        .collect();

    m_objectLayer->addChild(rp);
    m_fields->players.emplace(playerId, rp);
    m_fields->interpolator->addPlayer(playerId);

    log::debug("Player joined: {}", playerId);
}

void GlobedPlayLayer::handlePlayerLeave(int playerId) {
#if GLOBED_VOICE_SUPPORT
    VoicePlaybackManager::get().removeStream(playerId);
#endif // GLOBED_VOICE_SUPPORT

    if (!m_fields->players.contains(playerId)) return;

    m_fields->players.at(playerId)->removeFromParent();
    m_fields->players.erase(playerId);
    m_fields->interpolator->removePlayer(playerId);

    log::debug("Player removed: {}", playerId);
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

    m_fields->lastSentPacket.emplace(uniqueKey, now);

    return true;
}

void GlobedPlayLayer::unscheduleSelectors() {
    auto* sched = CCScheduler::get();
    sched->unscheduleSelector(schedule_selector(GlobedPlayLayer::selSendPlayerData), this->getParent());
    sched->unscheduleSelector(schedule_selector(GlobedPlayLayer::selPeriodicalUpdate), this->getParent());
}

void GlobedPlayLayer::rescheduleSelectors() {
    auto* sched = CCScheduler::get();
    float timescale = sched->getTimeScale();
    m_fields->lastKnownTimeScale = timescale;

    float pdInterval = (1.0f / m_fields->configuredTps) * timescale;
    float updpInterval = 0.25f * timescale;

    sched->scheduleSelector(schedule_selector(GlobedPlayLayer::selSendPlayerData), this->getParent(), pdInterval, false);
    sched->scheduleSelector(schedule_selector(GlobedPlayLayer::selPeriodicalUpdate), this->getParent(), updpInterval, false);
}
