#include "Events.hpp"
#include <dbuf/ByteReader.hpp>
#include <dbuf/ByteWriter.hpp>

using namespace geode::prelude;

namespace globed {

std::vector<uint8_t> TwoPlayerLinkEvent::encode() const {
    dbuf::ByteWriter<> writer;
    writer.writeI32(playerId);
    writer.writeBool(player1);
    return std::move(writer).intoInner();
}

Result<TwoPlayerLinkEvent> TwoPlayerLinkEvent::decode(std::span<const uint8_t> data) {
    dbuf::ByteReader reader(data);
    int playerId = GEODE_UNWRAP(reader.readI32());
    bool player1 = GEODE_UNWRAP(reader.readBool());

    return Ok(TwoPlayerLinkEvent { playerId, player1 });
}

std::vector<uint8_t> TwoPlayerUnlinkEvent::encode() const {
    dbuf::ByteWriter<> writer;
    writer.writeI32(playerId);
    return std::move(writer).intoInner();
}

Result<TwoPlayerUnlinkEvent> TwoPlayerUnlinkEvent::decode(std::span<const uint8_t> data) {
    dbuf::ByteReader reader(data);
    int playerId = GEODE_UNWRAP(reader.readI32());

    return Ok(TwoPlayerUnlinkEvent { playerId });
}

}
