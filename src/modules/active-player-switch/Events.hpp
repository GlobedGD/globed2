#pragma once
#include <globed/core/Event.hpp>

namespace globed {

struct APSFullStateEvent : ServerEvent<APSFullStateEvent> {
    static constexpr auto Type = EventServer::Game;
    static constexpr auto Id = "globed/switcheroo.full-state";

    APSFullStateEvent(int activePlayer, bool gameActive, bool playerIndication, bool restarting)
        : activePlayer(activePlayer), gameActive(gameActive), playerIndication(playerIndication), restarting(restarting) {}

    std::vector<uint8_t> encode() const;
    static geode::Result<APSFullStateEvent> decode(std::span<const uint8_t> data);

    int activePlayer = 0;
    bool gameActive = false;
    bool playerIndication = false;
    bool restarting = false;
};

struct APSSwitchEvent : ServerEvent<APSSwitchEvent> {
    static constexpr auto Type = EventServer::Game;
    static constexpr auto Id = "globed/switcheroo.switch";

    int playerId;
    enum SwitchType : uint8_t {
        Switch = 0,
        Warning = 1,
    } type;

    APSSwitchEvent(int playerId, enum SwitchType type) : playerId(playerId), type(type) {}

    static geode::Result<APSSwitchEvent> decode(std::span<const uint8_t> data);
    std::vector<uint8_t> encode() const;
};

}
