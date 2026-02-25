#pragma once

#include "APSModule.hpp"
#include <globed/prelude.hpp>
#include <globed/core/game/RemotePlayer.hpp>
#include <globed/core/net/MessageListener.hpp>
#include <globed/core/data/Messages.hpp>

#include <Geode/Geode.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>

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
    void handleStateEvent(const SwitcherooFullStateEvent& event);
    void handleSwitchEvent(const SwitcherooSwitchEvent& event);
    void rehidePlayers();

    std::optional<SwitcherooSwitchEvent> poll();

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
        MessageListener<msg::LevelDataMessage> m_listener;
        int m_myAccountId = 0;
        cocos2d::extension::CCScale9Sprite* m_switchGlow = nullptr;
        cocos2d::extension::CCScale9Sprite* m_switchPreglow = nullptr;
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
    void handleUpdate();
    void handleUpdateFromRp(PlayerObject* local, RemotePlayer* rp, bool p2);

    void customResetLevel();
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
            "GJBaseGameLayer::update"
        );
    }

    // TODO: it's better to hook queueButton but it's inlined
    // according to prevter handlebutton would break a bunch of bots and likely have incompat with cbf
    $override
    void handleButton(bool a, int b, bool c);

    $override
    void update(float dt);
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

}