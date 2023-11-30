#pragma once
#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>

#include <defs.hpp>

#if GLOBED_HAS_KEYBINDS
#include <geode.custom-keybinds/include/Keybinds.hpp>
#endif // GLOBED_CUSTOM_KEYBINDS

#include <audio/audio_manager.hpp>
#include <audio/voice_playback_manager.hpp>

#include <managers/profile_cache.hpp>
#include <managers/error_queues.hpp>
#include <net/network_manager.hpp>
#include <data/packets/all.hpp>
#include <util/math.hpp>

using namespace geode::prelude;

constexpr float DATA_SEND_INTERVAL = 1.0f / 30.f;
constexpr float DATA_SEND_INTERVAL_MS = DATA_SEND_INTERVAL * 1000;

class $modify(GlobedPlayLayer, PlayLayer) {
    bool globedReady;
    bool deafened = false;

    /* speedhack detection */
    float lastKnownTimeScale = 1.0f;
    chrono::system_clock::time_point lastSentPacket;

    /* gd hooks */

    bool init(GJGameLevel* level) {
        if (!PlayLayer::init(level)) return false;

        auto& nm = NetworkManager::get();

        // if not authenticated, do nothing
        m_fields->globedReady = nm.authenticated();
        if (!m_fields->globedReady) return true;

        // send LevelJoinPacket
        nm.send(LevelJoinPacket::create(m_level->m_levelID));

        // add listeners

        nm.addListener<PlayerProfilesPacket>([this](PlayerProfilesPacket* packet) {
            auto& pcm = ProfileCacheManager::get();
            for (auto& player : packet->data) {
                pcm.insert(player);
            }
        });

        nm.addListener<LevelDataPacket>([this](LevelDataPacket* packet){
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

        nm.addListener<PlayerMetadataPacket>([this](PlayerMetadataPacket* packet) {
            // TODO handle player metadata
        });

        // schedule stuff
        // TODO - handle sending SyncPlayerMetadataPacket.
        // it could be sent periodically as a lazy solution, but the best way
        // is to only do it when the state actually changed. like we got a new best or
        // the attempt count increased by quite a bit.

        auto scheduler = CCScheduler::get();
        scheduler->scheduleSelector(schedule_selector(GlobedPlayLayer::selSendPlayerData), this, DATA_SEND_INTERVAL, false);

        // setup custom keybinds
#if GLOBED_HAS_KEYBINDS
        // TODO let the user pick recording device somehow
        // TODO this breaks for impostor playlayers, if they won't be fixed in 2.2 then do a good old workaround
        this->addEventListener<keybinds::InvokeBindFilter>([=](keybinds::InvokeBindEvent* event) {
            auto& vm = GlobedAudioManager::get();

            if (event->isDown()) {
                if (!vm.isRecording()) {
                    // make sure the recording device is valid
                    vm.validateDevices();
                    if (!vm.isRecordingDeviceSet()) {
                        vm.setActiveRecordingDevice(2);
                        // ErrorQueues::get().warn("Unable to record audio, no recording device is set");
                        // return ListenerResult::Propagate;
                    }

                    vm.startRecording([this](const auto& frame){
                        this->audioCallback(frame);
                    });
                }
            } else {
                vm.stopRecording();
            }

            return ListenerResult::Propagate;
        }, "voice-activate"_spr);

        this->addEventListener<keybinds::InvokeBindFilter>([=](keybinds::InvokeBindEvent* event) {
            if (event->isDown()) {
                this->m_fields->deafened = !this->m_fields->deafened;
            }

            return ListenerResult::Propagate;
        }, "voice-deafen"_spr);
#endif // GLOBED_HAS_KEYBINDS

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

    /* periodical selectors */

    // selSendPlayerData - runs 30 times per second
    void selSendPlayerData(float _dt) {
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

    void audioCallback(const EncodedAudioFrame& frame) {
        if (!this->established()) return;

        auto& nm = NetworkManager::get();

        // we only have a const ref to EncodedAudioFrame,
        // but we need to be able to access it inside the NetworkManager thread.
        // so we encode it to a temporary buffer and use RawPacket.
        // this is inefficient and copies data but ehhh

        ByteBuffer buf;
        buf.writeValue(frame);

        nm.send(RawPacket::create(VoicePacket::PACKET_ID, VoicePacket::ENCRYPTED, std::move(buf)));
    }

    // With speedhack enabled, all scheduled selectors will run more often than they are supposed to.
    // This means, if you turn up speedhack to let's say 100x, you will send 3000 packets per second. That is a big no-no.
    // For naive speedhack implementations, we simply check CCScheduler::getTimeScale and properly reschedule our data sender.
    //
    // For non-naive speedhacks however, ones that don't use CCScheduler::setTimeScale, it is more complicated.
    // We record the time of sending each packet and compare the intervals. If the interval is suspiciously small, we reject the packet.
    // This does result in less smooth experience with non-naive speedhacks however.
    bool accountForSpeedhack() {
        auto sched = CCScheduler::get();
        float ts = sched->getTimeScale();
        if (!util::math::equal(ts, m_fields->lastKnownTimeScale)) {
            m_fields->lastKnownTimeScale = ts;
            auto sel = schedule_selector(GlobedPlayLayer::selSendPlayerData);
            sched->unscheduleSelector(sel, this);
            sched->scheduleSelector(sel, this, DATA_SEND_INTERVAL * ts, false);
        }

        auto now = util::time::now();
        auto passed = chrono::duration_cast<chrono::milliseconds>(now - m_fields->lastSentPacket).count();

        if (passed < DATA_SEND_INTERVAL_MS * 0.85f) {
            // log::warn("dropping a packet (speedhack?), passed time is {}ms", passed);
            return false;
        }

        m_fields->lastSentPacket = now;

        return true;
    }

    // remove if impostor playlayer gets fixed
    bool isCurrentPlayLayer() {
        auto playLayer = getChildOfType<PlayLayer>(CCScene::get(), 0);
        return playLayer == this;
    }
};