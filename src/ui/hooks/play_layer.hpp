#pragma once
#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>

#include <audio/audio_manager.hpp>
#include <audio/voice_playback_manager.hpp>

#include <managers/profile_cache.hpp>
#include <net/network_manager.hpp>
#include <data/packets/all.hpp>

using namespace geode::prelude;

class $modify(GlobedPlayLayer, PlayLayer) {
    bool globedReady;

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
            // TODO client side blocking and stuff..
            VoicePlaybackManager::get().playFrameStreamed(packet->sender, packet->frame);
        });

        // schedule stuff

        auto scheduler = CCScheduler::get();
        scheduler->scheduleSelector(schedule_selector(GlobedPlayLayer::sendPlayerData), this, 1.0f / 30.f, false);

        return true;
    }

    void onQuit() {
        PlayLayer::onQuit();

        if (!m_fields->globedReady) return;

        auto& nm = NetworkManager::get();

        // send LevelLeavePacket
        nm.send(LevelLeavePacket::create());

        // clean up the listeners
        nm.removeListener<PlayerProfilesPacket>();
        nm.removeListener<LevelDataPacket>();
        nm.removeListener<VoiceBroadcastPacket>();
    }

    /* periodical selectors */

    // sendPlayerData - runs 30 times per second
    void sendPlayerData(float _dt) {
        auto& nm = NetworkManager::get();
        auto data = this->gatherPlayerData();

        nm.send(PlayerDataPacket::create(data));
    }

    /* private utilities */

    PlayerData gatherPlayerData() {
        return PlayerData();
    }
};