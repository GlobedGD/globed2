#pragma once
#include <globed/core/Event.hpp>

namespace globed {

struct CounterChangeEvent : ServerEvent<CounterChangeEvent, EventServer::Game> {
    static constexpr auto Id = "globed/counter-change";

    CounterChangeEvent() = default;
    CounterChangeEvent(uint8_t rawType, uint32_t itemId, uint32_t rawValue)
        : rawType(rawType), itemId(itemId), rawValue(rawValue) {}

    uint8_t rawType;
    uint32_t itemId;
    uint32_t rawValue;

    std::vector<uint8_t> encode() const;
    static Result<CounterChangeEvent> decode(std::span<const uint8_t> data);
};

}
