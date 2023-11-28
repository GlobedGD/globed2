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

using namespace geode::prelude;

class $modify(GlobedPlayLayer, PlayLayer) {
    bool globedReady;
    bool deafened = false;

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
            VoicePlaybackManager::get().playFrameStreamed(packet->sender, packet->frame);

        });

        // schedule stuff

        auto scheduler = CCScheduler::get();
        scheduler->scheduleSelector(schedule_selector(GlobedPlayLayer::selSendPlayerData), this, 1.0f / 30.f, false);

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

        auto& nm = NetworkManager::get();

        auto data = this->gatherPlayerData();

        nm.send(PlayerDataPacket::create(data));
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
        VoicePlaybackManager::get().prepareStream(playerId);
    }

    void handlePlayerLeave(int playerId) {
        VoicePlaybackManager::get().removeStream(playerId);
    }

    void audioCallback(const EncodedAudioFrame& frame) {
        if (!this->established()) return;

        log::debug("audio callback with {} frames", frame.size());

        auto& nm = NetworkManager::get();

        // we only have a const ref to EncodedAudioFrame,
        // but we need to be able to access it inside the NetworkManager thread.
        // so we encode it to a temporary buffer and use RawPacket.
        // this is inefficient and copies data but ehhh

        ByteBuffer buf;
        buf.writeValue(frame);

        nm.send(RawPacket::create(VoicePacket::PACKET_ID, VoicePacket::ENCRYPTED, std::move(buf)));
    }
};