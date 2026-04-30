#pragma once
#include <globed/core/Event.hpp>
#include <dbuf/ByteReader.hpp>

namespace globed {

struct TwoPlayerLinkEvent : ServerEvent<TwoPlayerLinkEvent, EventServer::Game> {
    static constexpr auto Id = "globed/2p.link";

    TwoPlayerLinkEvent(int playerId, bool player1) : playerId(playerId), player1(player1) {}

    std::vector<uint8_t> encode() const;
    static geode::Result<TwoPlayerLinkEvent> decode(std::span<const uint8_t> data);

    int playerId;
    bool player1;
};

struct TwoPlayerUnlinkEvent : ServerEvent<TwoPlayerUnlinkEvent, EventServer::Game> {
    static constexpr auto Id = "globed/2p.unlink";

    int playerId;
    TwoPlayerUnlinkEvent(int playerId) : playerId(playerId) {}

    static geode::Result<TwoPlayerUnlinkEvent> decode(std::span<const uint8_t> data);
    std::vector<uint8_t> encode() const;
};

}
