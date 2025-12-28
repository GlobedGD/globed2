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

struct GLOBED_MODIFY_ATTR APSPlayLayer : geode::Modify<APSPlayLayer, PlayLayer> {
    struct Fields {
        std::optional<MessageListener<msg::LevelDataMessage>> m_listener;
        int m_activePlayer = 0;
        int m_switchingTo = 0;
        bool m_meActive = false;
        cocos2d::extension::CCScale9Sprite* m_switchGlow = nullptr;
        cocos2d::extension::CCScale9Sprite* m_switchPreglow = nullptr;
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

    void handleEvent(const ActivePlayerSwitchEvent& event);
    void handlePlayerDeath(const PlayerDeath& death, RemotePlayer* player);
    void handleUpdate();
    void handleUpdateFromRp(PlayerObject* local, RemotePlayer* rp, bool p2);

    void customResetLevel();
    bool shouldBlockInput();
    void performSwitch();
    void performPreSwitch();
    void performSwitchProxy(float dt);
    void performPreSwitchProxy(float dt);

    void showSwitchEffect();
    void showPreSwitchEffect();
    void showEffect(bool presw);
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