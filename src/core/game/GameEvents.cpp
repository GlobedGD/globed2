#include <globed/core/game/GameEvents.hpp>
#include <dbuf/ByteWriter.hpp>
#include <dbuf/ByteReader.hpp>

using namespace geode::prelude;

namespace globed {

std::vector<uint8_t> DisplayDataRefreshedEvent::encode() const {
    dbuf::ByteWriter wr;
    wr.writeI32(playerId);
    auto written = wr.written();

    std::vector<uint8_t> out;
    out.assign_range(wr.written());
    return out;
}

Result<DisplayDataRefreshedEvent> DisplayDataRefreshedEvent::decode(std::span<const uint8_t> data) {
    dbuf::ByteReader reader{data};
    DisplayDataRefreshedEvent out{};
    out.playerId = GEODE_UNWRAP(reader.readI32());
    return Ok(out);
}

}
