#pragma once
#include <globed/core/Event.hpp>
#include <dbuf/ByteReader.hpp>

namespace globed {

struct TwoPlayerLinkEvent : ServerEvent<TwoPlayerLinkEvent, EventServer::Game> {
    static constexpr auto Id = "globed/2p.link";

    TwoPlayerLinkEvent(bool player1) : player1(player1) {}

    std::vector<uint8_t> encode() const;
    static geode::Result<TwoPlayerLinkEvent> decode(std::span<const uint8_t> data);

    bool player1;
};

struct TwoPlayerUnlinkEvent : ServerEvent<TwoPlayerUnlinkEvent, EventServer::Game> {
    static constexpr auto Id = "globed/2p.unlink";

    TwoPlayerUnlinkEvent() = default;

    static geode::Result<TwoPlayerUnlinkEvent> decode(std::span<const uint8_t> data) {
        return Ok(TwoPlayerUnlinkEvent {});
    }

    std::vector<uint8_t> encode() const { return {}; }
};

}
