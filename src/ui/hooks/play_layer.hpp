#pragma once
#include <defs.hpp>

#include <Geode/modify/PlayLayer.hpp>
#if GLOBED_HAS_KEYBINDS
# include <geode.custom-keybinds/include/Keybinds.hpp>
#endif // GLOBED_HAS_KEYBINDS

#include <audio/all.hpp>
#include <managers/profile_cache.hpp>
#include <managers/error_queues.hpp>
#include <managers/settings.hpp>
#include <net/network_manager.hpp>
#include <data/packets/all.hpp>
#include <ui/game/player/remote_player.hpp>
#include <util/math.hpp>
#include <util/debugging.hpp>

using namespace geode::prelude;

class $modify(GlobedPlayLayer, PlayLayer) {
    // setup stuff
    GlobedSettings& settings = GlobedSettings::get();
    bool globedReady;
    uint32_t configuredTps = 0;

    // in game stuff
    bool deafened = false;
    uint32_t totalSentPackets = 0;
    std::unordered_map<int, RemotePlayer*> players;
    float timeCounter = 0.f;

    // speedhack detection
    float lastKnownTimeScale = 1.0f;
    std::unordered_map<int, util::time::time_point> lastSentPacket;

    // gd hooks

    bool init(GJGameLevel* level, bool p1, bool p2) {
        if (!PlayLayer::init(level, p1, p2)) return false;

        auto& nm = NetworkManager::get();

        // if not authenticated, do nothing
        m_fields->globedReady = nm.established();
        if (!m_fields->globedReady) return true;

        // set the configured tps
        auto tpsCap = m_fields->settings.globed.tpsCap;
        if (tpsCap != 0) {
            m_fields->configuredTps = std::min(nm.connectedTps.load(), tpsCap);
        } else {
            m_fields->configuredTps = nm.connectedTps;
        }

#if GLOBED_VOICE_SUPPORT
        // set the audio device
        try {
            GlobedAudioManager::get().setActiveRecordingDevice(m_fields->settings.globed.audioDevice);
        } catch(const std::exception& e) {
            // try default device, if we have no mic then just do nothing
            try {
                GlobedAudioManager::get().setActiveRecordingDevice(0);
            } catch(const std::exception& _e) {}
        }
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

        this->rescheduleSelectors();

        CCScheduler::get()->scheduleSelector(schedule_selector(GlobedPlayLayer::selIncreaseCounter), this, 0.0f, false);

        return true;
    }

    void onQuit() {
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
    }

    /* setup stuff to make init() cleaner */

    void setupPacketListeners() {
        auto& nm = NetworkManager::get();

        nm.addListener<PlayerProfilesPacket>([](PlayerProfilesPacket* packet) {
            auto& pcm = ProfileCacheManager::get();
            for (auto& player : packet->players) {
                pcm.insert(player);
            }
        });

        nm.addListener<LevelDataPacket>([this](LevelDataPacket* packet){
            for (const auto& player : packet->players) {
                if (!this->m_fields->players.contains(player.accountId)) {
                    // new player joined
                    this->handlePlayerJoin(player.accountId);
                }

                this->m_fields->players.at(player.accountId)->updateData(player.data, m_fields->timeCounter);
            }
        });

        nm.addListener<VoiceBroadcastPacket>([this](VoiceBroadcastPacket* packet) {
#if GLOBED_VOICE_SUPPORT
            // if deafened or voice is disabled, do nothing
            if (this->m_fields->deafened || !this->m_fields->settings.communication.voiceEnabled) return;

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

    void setupCustomKeybinds() {
#if GLOBED_HAS_KEYBINDS && GLOBED_VOICE_SUPPORT
        // the only keybinds used are for voice chat, so if voice is disabled, do nothing
        if (!m_fields->settings.communication.voiceEnabled) {
            return;
        }

        // TODO this breaks for impostor playlayers, if they won't be fixed in 2.2 then do a good old workaround
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

                    vm.startRecording([](const auto& frame) {
                        // (!) remember that the callback is ran from another thread (!) //

                        auto& nm = NetworkManager::get();
                        if (!nm.established()) return;

                        // `frame` does not live long enough and will be destructed at the end of this callback.
                        // so we can't pass it directly in a `VoicePacket` and we use a `RawPacket` instead.

                        ByteBuffer buf;
                        buf.writeValue(frame);

                        nm.send(RawPacket::create(VoicePacket::PACKET_ID, VoicePacket::ENCRYPTED, std::move(buf)));
                    });
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

    /* periodical selectors */

    // selSendPlayerData - runs 30 times per second
    void selSendPlayerData(float) {
        if (!this->established()) return;
        if (!this->isCurrentPlayLayer()) return;
        if (!this->accountForSpeedhack(0, 1.0f / m_fields->configuredTps, 0.8f)) return;

        m_fields->totalSentPackets++;
        // additionally, if there are no players on the level, we drop down to 1 time per second as an optimization
        if (m_fields->players.empty() && m_fields->totalSentPackets % 30 != 15) return;

        auto data = this->gatherPlayerData();
        NetworkManager::get().send(PlayerDataPacket::create(data));
    }

    // selUpdateProfiles - runs 4 times a second, updates profiles of other people and removes dead players
    void selUpdateProfiles(float) {
        if (!this->established()) return;
        if (!this->isCurrentPlayLayer()) return;

        auto& pcm = ProfileCacheManager::get();

        std::vector<int> toRemove;

        for (const auto [playerId, remotePlayer] : m_fields->players) {
            if (m_fields->timeCounter - remotePlayer->updateCounter > 0.75f && remotePlayer->updateCounter != 0.f) {
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
            this->handlePlayerLeave(id);
        }
    }

    // selIncreaseCounter - runs every frame and increments the non-decreasing time counter
    void selIncreaseCounter(float dt) {
        m_fields->timeCounter += dt;
    }

    /* private utilities */

    bool established() {
        // the 2nd check is in case we disconnect while being in a level somehow
        return m_fields->globedReady && NetworkManager::get().established();
    }

    SpecificIconData gatherSpecificIconData(PlayerObject* player) {
        PlayerIconType iconType = PlayerIconType::Cube;
        if (player->m_isShip) iconType = PlayerIconType::Ship;
        else if (player->m_isBird) iconType = PlayerIconType::Ufo;
        else if (player->m_isBall) iconType = PlayerIconType::Ball;
        else if (player->m_isDart) iconType = PlayerIconType::Wave;
        else if (player->m_isRobot) iconType = PlayerIconType::Robot;
        else if (player->m_isSpider) iconType = PlayerIconType::Spider;
        else if (player->m_isSwing) iconType = PlayerIconType::Swing;

        return SpecificIconData {
            .iconType = iconType,
            .position = player->m_position, // TODO maybe use getPosition ?
            .rotation = player->getRotation(),
            .isVisible = player->isVisible(),
        };
    }

    PlayerData gatherPlayerData() {
        return PlayerData(
            m_level->m_normalPercent,
            m_level->m_attempts,
            this->gatherSpecificIconData(m_player1),
            this->gatherSpecificIconData(m_player2) // TODO detect isDualMode
        );
    }

    void handlePlayerJoin(int playerId) {
#if GLOBED_VOICE_SUPPORT
        try {
            VoicePlaybackManager::get().prepareStream(playerId);
        } catch (const std::exception& e) {
            ErrorQueues::get().error(std::string("Failed to prepare audio stream: ") + e.what());
        }
#endif // GLOBED_VOICE_SUPPORT
        NetworkManager::get().send(RequestPlayerProfilesPacket::create(playerId));

        auto* rp = RemotePlayer::create();
        rp->setZOrder(10); // TODO temp

        auto nodeid = fmt::format("remote-player-{}", playerId);

        rp->setID(Mod::get()->expandSpriteName(nodeid.c_str()));

        m_objectLayer->addChild(rp);
        m_fields->players.emplace(playerId, rp);

        log::debug("Player joined: {}", playerId);
    }

    void handlePlayerLeave(int playerId) {
#if GLOBED_VOICE_SUPPORT
        VoicePlaybackManager::get().removeStream(playerId);
#endif // GLOBED_VOICE_SUPPORT

        if (!m_fields->players.contains(playerId)) return;

        m_fields->players.at(playerId)->removeFromParent();
        m_fields->players.erase(playerId);

        log::debug("Player removed: {}", playerId);
    }

    // With speedhack enabled, all scheduled selectors will run more often than they are supposed to.
    // This means, if you turn up speedhack to let's say 100x, you will send 3000 packets per second. That is a big no-no.
    // For naive speedhack implementations, we simply check CCScheduler::getTimeScale and properly reschedule our data sender.
    //
    // For non-naive speedhacks however, ones that don't use CCScheduler::setTimeScale, it is more complicated.
    // We record the time of sending each packet and compare the intervals. If the interval is suspiciously small, we reject the packet.
    // This does result in less smooth experience with non-naive speedhacks however.
    bool accountForSpeedhack(size_t uniqueKey, float cap, float allowance = 0.9f) { // TODO test if this still works for classic speedhax
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

    void unscheduleSelectors() {
        auto* sched = CCScheduler::get();
        sched->unscheduleSelector(schedule_selector(GlobedPlayLayer::selSendPlayerData), this);
        sched->unscheduleSelector(schedule_selector(GlobedPlayLayer::selUpdateProfiles), this);
    }

    void rescheduleSelectors() {
        auto* sched = CCScheduler::get();
        float timescale = sched->getTimeScale();
        m_fields->lastKnownTimeScale = timescale;

        float pdInterval = (1.0f / m_fields->configuredTps) * timescale;
        float updpInterval = 0.25f * timescale;

        sched->scheduleSelector(schedule_selector(GlobedPlayLayer::selSendPlayerData), this, pdInterval, false);
        sched->scheduleSelector(schedule_selector(GlobedPlayLayer::selUpdateProfiles), this, updpInterval, false);
    }

    // TODO remove if impostor playlayer gets fixed
    bool isCurrentPlayLayer() {
        auto playLayer = getChildOfType<PlayLayer>(CCScene::get(), 0);
        return playLayer == this;
    }

    bool isPaused() {
        return this->getParent()->getChildByID("PauseLayer") != nullptr; // TODO no worky on android and relies on node ids from geode
    }
};