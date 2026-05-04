#include "Events.hpp"
#include <dbuf/ByteReader.hpp>
#include <dbuf/ByteWriter.hpp>

using namespace geode::prelude;

namespace globed {

std::vector<uint8_t> CounterChangeEvent::encode() const {
    uint64_t rawData =
        (static_cast<uint64_t>(rawType) << 56)
        | ((static_cast<uint64_t>(itemId) & 0x00ffffffull) << 32);

    rawData |= rawValue;


    dbuf::ByteWriter<> writer;
    writer.writeU64(rawData);
    return std::move(writer).intoInner();
}

Result<CounterChangeEvent> CounterChangeEvent::decode(std::span<const uint8_t> data) {
    dbuf::ByteReader<> reader(data);
    uint64_t rawData = GEODE_UNWRAP(reader.readU64());

    uint8_t rawType = (rawData >> 56) & 0xff;
    uint32_t itemId = (rawData >> 32) & 0x00fffff;
    uint32_t value = static_cast<uint32_t>(rawData & 0xffffffffULL);

    return Ok(CounterChangeEvent {
        rawType, itemId, value
    });
}

}
