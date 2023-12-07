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
#include <util/math.hpp>

using namespace geode::prelude;

class $modify(GlobedPlayLayer, PlayLayer) {
    bool globedReady;
    bool deafened = false;
    GlobedSettings::CachedSettings settings;
    uint32_t configuredTps = 0;

    /* speedhack detection */
    float lastKnownTimeScale = 1.0f;
    util::time::time_point lastSentPacket;

    /* gd hooks */

    bool init(GJGameLevel* level) {
        if (!PlayLayer::init(level)) return false;

        m_fields->settings = GlobedSettings::get().getCached();

        auto& nm = NetworkManager::get();

        // if not authenticated, do nothing
        m_fields->globedReady = nm.established();
        if (!m_fields->globedReady) return true;

        auto tpsCap = m_fields->settings.tpsCap;

        if (tpsCap != 0) {
            m_fields->configuredTps = std::min(nm.connectedTps.load(), tpsCap);
        } else {
            m_fields->configuredTps = nm.connectedTps;
        }

        // send LevelJoinPacket
        nm.send(LevelJoinPacket::create(m_level->m_levelID));

        this->setupEventListeners();

        GlobedAudioManager::get().setActiveRecordingDevice(2);

        // schedule stuff
        // TODO - handle sending SyncPlayerMetadataPacket.
        // it could be sent periodically as a lazy solution, but the best way
        // is to only do it when the state actually changed. like we got a new best or
        // the attempt count increased by quite a bit.

        this->rescheduleSender();

        this->setupCustomKeybinds();

        return true;
    }

    void onQuit() {
        PlayLayer::onQuit();

        if (!m_fields->globedReady) return;

        GlobedAudioManager::get().haltRecording();
        VoicePlaybackManager::get().stopAllStreams();

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

    void setupEventListeners() {
        auto& nm = NetworkManager::get();

        nm.addListener<PlayerProfilesPacket>([](PlayerProfilesPacket* packet) {
            auto& pcm = ProfileCacheManager::get();
            for (auto& player : packet->data) {
                pcm.insert(player);
            }
        });

        nm.addListener<LevelDataPacket>([](LevelDataPacket* packet){
            log::debug("Recv level data, {} players", packet->players.size());
        });

        nm.addListener<VoiceBroadcastPacket>([this](VoiceBroadcastPacket* packet) {
            if (this->m_fields->deafened) return;

            // TODO client side blocking and stuff..
            log::debug("streaming frame from {}", packet->sender);
            // TODO - this decodes the sound data on the main thread. might be a bad idea, will need to benchmark.
            try {
                VoicePlaybackManager::get().playFrameStreamed(packet->sender, packet->frame);
            } catch (const std::exception& e) {
                ErrorQueues::get().debugWarn(std::string("Failed to play a voice frame: ") + e.what());
            }
        });

        nm.addListener<PlayerMetadataPacket>([](PlayerMetadataPacket*) {
            // TODO handle player metadata
        });
    }

    void setupCustomKeybinds() {
#if GLOBED_HAS_KEYBINDS
        // TODO let the user pick recording device somehow
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
#endif // GLOBED_HAS_KEYBINDS
    }

    /* periodical selectors */

    // selSendPlayerData - runs 30 times per second
    void selSendPlayerData(float) {
        if (!this->established()) return;
        if (!this->isCurrentPlayLayer()) return;
        if (!this->accountForSpeedhack()) return;

        auto data = this->gatherPlayerData();
        NetworkManager::get().send(PlayerDataPacket::create(data));
    }

    /* private utilities */

    bool established() {
        // the 2nd check is in case we disconnect while being in a level somehow
        return m_fields->globedReady && NetworkManager::get().established();
    }

    PlayerData gatherPlayerData() {
        return PlayerData();
    }

    void handlePlayerJoin(int playerId) {
        try {
            VoicePlaybackManager::get().prepareStream(playerId);
        } catch (const std::exception& e) {
            ErrorQueues::get().error(std::string("Failed to prepare audio stream: ") + e.what());
        }
    }

    void handlePlayerLeave(int playerId) {
        VoicePlaybackManager::get().removeStream(playerId);
    }

    // With speedhack enabled, all scheduled selectors will run more often than they are supposed to.
    // This means, if you turn up speedhack to let's say 100x, you will send 3000 packets per second. That is a big no-no.
    // For naive speedhack implementations, we simply check CCScheduler::getTimeScale and properly reschedule our data sender.
    //
    // For non-naive speedhacks however, ones that don't use CCScheduler::setTimeScale, it is more complicated.
    // We record the time of sending each packet and compare the intervals. If the interval is suspiciously small, we reject the packet.
    // This does result in less smooth experience with non-naive speedhacks however.
    bool accountForSpeedhack() {
        auto* sched = CCScheduler::get();
        auto ts = sched->getTimeScale();
        if (util::math::equal(ts, m_fields->lastKnownTimeScale)) {
            sched->unscheduleSelector(schedule_selector(GlobedPlayLayer::selSendPlayerData), this);
            this->rescheduleSender();
        }

        auto now = util::time::now();
        auto passed = util::time::asMillis(now - m_fields->lastSentPacket);

        float cap = (1.0f / m_fields->configuredTps) * 1000;

        if (passed < cap * 0.85f) {
            // log::warn("dropping a packet (speedhack?), passed time is {}ms", passed);
            return false;
        }

        m_fields->lastSentPacket = now;

        return true;
    }

    void rescheduleSender() {
        auto* sched = CCScheduler::get();
        float timescale = sched->getTimeScale();
        float interval = (1.0f / m_fields->configuredTps) * timescale;
        m_fields->lastKnownTimeScale = timescale;
        sched->scheduleSelector(schedule_selector(GlobedPlayLayer::selSendPlayerData), this, interval, false);
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