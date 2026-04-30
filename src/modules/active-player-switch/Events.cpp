#include "Events.hpp"

using namespace geode::prelude;

namespace globed {

std::vector<uint8_t> APSFullStateEvent::encode() const {
    dbuf::ByteWriter<> writer;

    writer.writeI32(activePlayer);

    uint8_t flags = 0;
    if (this->gameActive) flags |= 0b00000001;
    if (this->playerIndication) flags |= 0b00000010;
    if (this->restarting) flags |= 0b00000100;

    writer.writeU8(flags);

    return std::move(writer).intoInner();
}

Result<APSFullStateEvent> APSFullStateEvent::decode(std::span<const uint8_t> data) {
    dbuf::ByteReader reader(data);
    int activePlayer = GEODE_UNWRAP(reader.readI32());
    uint8_t flags = GEODE_UNWRAP(reader.readU8());

    bool active = (flags & 0b00000001) != 0;
    bool indication = (flags & 0b00000010) != 0;
    bool restart = (flags & 0b00000100) != 0;

    return Ok(APSFullStateEvent {
        activePlayer, active, indication, restart
    });
}

std::vector<uint8_t> APSSwitchEvent::encode() const {
    dbuf::ByteWriter<> writer;
    writer.writeI32(playerId);
    writer.writeU8(type);
    return std::move(writer).intoInner();
}

Result<APSSwitchEvent> APSSwitchEvent::decode(std::span<const uint8_t> data) {
    dbuf::ByteReader reader(data);
    auto playerId = GEODE_UNWRAP(reader.readI32());
    auto type = GEODE_UNWRAP(reader.readU8());

    return Ok(APSSwitchEvent { playerId, SwitchType{type} });
}

}
