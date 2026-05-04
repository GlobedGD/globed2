#include "Events.hpp"
#include <dbuf/ByteReader.hpp>
#include <dbuf/ByteWriter.hpp>

using namespace geode::prelude;

namespace globed {

std::vector<uint8_t> TwoPlayerLinkEvent::encode() const {
    return { (uint8_t)player1 };
}

Result<TwoPlayerLinkEvent> TwoPlayerLinkEvent::decode(std::span<const uint8_t> data) {
    dbuf::ByteReader reader(data);
    bool player1 = GEODE_UNWRAP(reader.readBool());
    return Ok(TwoPlayerLinkEvent { player1 });
}

}
