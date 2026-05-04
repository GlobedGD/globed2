#pragma once

#include "APSModule.hpp"
#include "Events.hpp"
#include <globed/prelude.hpp>
#include <globed/core/game/RemotePlayer.hpp>

#include <Geode/Geode.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>

namespace globed {

enum class APSSelectionAlgorithm {
    /// Truly random selection of the next player (minus the active one)
    Random,
    /// Tetris-like selection, every player gets to play in a single cycle, but the order is random
    FairRandom,
    /// Deterministic order based on account ID
    Sequential,
};

struct APSSettings {
    APSSelectionAlgorithm m_cycleAlgo = APSSelectionAlgorithm::FairRandom;
    float m_interval = 5.f;
    float m_intervalVar = 2.f;
    float m_warningDelay = 1.f;
    bool m_showNextPlayer = true;
};

struct APSPlayLayer;
struct APSController {
    APSSettings m_settings{};
    APSPlayLayer* m_pl{};
    GlobedGJBGL* m_gjbgl{};
    int m_activePlayer = 0;
    int m_nextPlayer = 0;
    bool m_meActive = false;
    bool m_gameActive = false;

    void restart();
    void handleStateEvent(const APSFullStateEvent& event, int sender);
    void handleSwitchEvent(const APSSwitchEvent& event, int sender);
    void rehidePlayers();
    void repushButtons();
    void rescheduleNextSwitch(float delayS);

    std::optional<APSSwitchEvent> poll();

private:
    std::vector<int> m_pqueue;
    asp::Instant m_nextSwitch;
    int m_detNext = 0;
    bool m_warned = false;

    void calculateNextSwitch();
    int pickNextPlayer();
    void getAllPlayers();
};

struct GLOBED_MODIFY_ATTR APSPlayLayer : geode::Modify<APSPlayLayer, PlayLayer> {
    struct Fields {
        geode::ListenerHandle m_fullStateListener, m_switchListener;
        int m_myAccountId = 0;
        geode::NineSlice* m_switchGlow = nullptr;
        geode::NineSlice* m_switchPreglow = nullptr;
        APSController m_controller{};
        bool m_controlling = false; // whether we are room owner
    };

    static void onModify(auto& self) {
        GLOBED_CLAIM_HOOKS(APSModule::get(), self,
            "PlayLayer::init",
            "PlayLayer::destroyPlayer"
        );
    }

    static APSPlayLayer* get(GJBaseGameLayer* gjbgl = nullptr);

    $override
    bool init(GJGameLevel* level, bool a, bool b);

    $override
    void destroyPlayer(PlayerObject* player, GameObject* obj);

    void handlePlayerDeath(const PlayerDeath& death, RemotePlayer* player);
    void handleUpdate(float dt);
    void handleUpdateFromRp(PlayerObject* local, RemotePlayer* rp, bool p2);

    bool shouldBlockInput();

    void showSwitchEffect();
    void showPreSwitchEffect();
    void showEffect(bool presw);
    void hideSwitchEffects();
    void showNextPlayerNotification(int id);

    void restartSwitchCycle();
    void sendFullState(bool restarting = false);
    void updateSettings(const APSSettings& settings);
};

struct GLOBED_MODIFY_ATTR APSGJBGL : geode::Modify<APSGJBGL, GJBaseGameLayer> {
    static void onModify(auto& self) {
        GLOBED_CLAIM_HOOKS(APSModule::get(), self,
            "GJBaseGameLayer::handleButton",
        );
    }

    // TODO: it's better to hook queueButton but it's inlined
    // according to prevter handlebutton would break a bunch of bots and likely have incompat with cbf
    $override
    void handleButton(bool a, int b, bool c);

    $override
    void processQueuedButtons(float dt, bool clearInputQueue);
};

struct GLOBED_MODIFY_ATTR APSPauseLayer : geode::Modify<APSPauseLayer, PauseLayer> {
    static void onModify(auto& self) {
        GLOBED_CLAIM_HOOKS(APSModule::get(), self,
            "PauseLayer::customSetup"
        );
    }

    $override
    void customSetup();
};

struct GLOBED_MODIFY_ATTR APSPlayerObject : geode::Modify<APSPlayerObject, PlayerObject> {
    static void onModify(auto& self) {
        GLOBED_CLAIM_HOOKS(APSModule::get(), self,
            "PlayerObject::update"
        );
    }

    $override
    void update(float dt);
};

}