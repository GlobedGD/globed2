#pragma once

#include "../Event.hpp"

namespace globed {

struct DisplayDataRefreshedEvent : ServerEvent<DisplayDataRefreshedEvent, EventServer::Game> {
    static constexpr auto Id = "globed/display-data-refreshed";

    std::vector<uint8_t> encode() const;
    static geode::Result<DisplayDataRefreshedEvent> decode(std::span<const uint8_t> data);

    int playerId = 0;
};

}
