#pragma once

#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <globed/config.hpp>
#include <globed/core/data/Messages.hpp>
#include <globed/core/net/MessageListener.hpp>
#include "../GlobalTriggersModule.hpp"
#include "../CounterChange.hpp"

namespace globed {

struct GLOBED_DLL GLOBED_NOVTABLE GTriggersGJBGL : geode::Modify<GTriggersGJBGL, GJBaseGameLayer> {
    struct Fields {
        std::optional<MessageListener<msg::LevelDataMessage>> m_listener;
        std::unordered_map<int, bool> m_pausedPlayers;

        int m_totalJoins = 0, m_totalLeaves = 0;
        bool m_firstPacket = true;
    };

    static void onModify(auto& self) {
        GLOBED_CLAIM_HOOKS(GlobalTriggersModule::get(), self,
            "GJBaseGameLayer::activateItemEditTrigger",
            // "GJBaseGameLayer::activateItemCompareTrigger",
            "GJBaseGameLayer::getItemValue",
        );
    }

    static GTriggersGJBGL* get(GJBaseGameLayer* base = nullptr);

    std::optional<int> activateItemEditTriggerReimpl(ItemTriggerGameObject* obj);

    $override
    void activateItemEditTrigger(ItemTriggerGameObject* obj);

    $override
    double getItemValue(int a, int b);

    void postInit();
    void registerListener();
    void registerSchedules();

    void handlePlayerJoin(int accountId);
    void handlePlayerLeave(int accountId);
    void recordPlayerPause(int accountId);

    void updateItems(float dt);

    void handleEvent(const Event& event);
    void applyCounterChange(const CounterChange& change);

    void updateCustomItem(int itemId, int value);
};

}
