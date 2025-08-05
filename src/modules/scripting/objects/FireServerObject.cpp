#include "FireServerObject.hpp"

#include <qunet/buffers/HeapByteWriter.hpp> // TODO: ByteWriter instead when implemented
#include <qunet/buffers/ByteReader.hpp>

using namespace geode::prelude;

namespace globed {

FireServerObject::FireServerObject() {}

void FireServerObject::triggerObject(GJBaseGameLayer* p0, int p1, gd::vector<int> const* p2) {
    EffectGameObject::triggerObject(p0, p1, p2);
    log::debug("Fire emoji");
}

static uint8_t computeChecksum(std::span<const uint8_t> data) {
    uint16_t sum = 0;
    for (auto byte : data) {
        sum += byte;
    }

    while (sum >> 8) {
        sum = (sum & 0xFF) + (sum >> 8);
    }

    return ~sum & 0xff;
}

static Result<FireServerPayload, qn::ByteReaderError> decodePayload(std::span<const uint8_t> data) {
    qn::ByteReader reader{data};

    FireServerPayload out{};
    out.eventId = GEODE_UNWRAP(reader.readU16());
    out.argCount = GEODE_UNWRAP(reader.readU8());

    uint8_t argt = 0;

    for (size_t i = 0; i < out.argCount; i++) {
        auto& outArg = out.args[i];

        if (i % 2 == 0) {
            argt = GEODE_UNWRAP(reader.readU8());
            outArg.type = static_cast<FireServerArgType>(argt >> 4);
        } else {
            outArg.type = static_cast<FireServerArgType>(argt & 0xf);
        }
    }

    for (size_t i = 0; i < out.argCount; i++) {
        out.args[i].rawValue = GEODE_UNWRAP(reader.readU32());
    }

    return Ok(out);
}

static void encodePayload(const FireServerPayload& payload, qn::HeapByteWriter& writer) {
    writer.writeU16(payload.eventId);
    writer.writeU8(payload.argCount);

    // encode the argument types, 1 byte holds two 4-bit values

    uint8_t argt = 0;

    for (size_t i = 0; i < payload.argCount; i++) {
        auto& arg = payload.args[i];

        if (i % 2 == 0) {
            argt = static_cast<uint8_t>(arg.type) << 4;
        } else {
            argt |= static_cast<uint8_t>(arg.type);
            writer.writeU8(argt);
        }
    }

    if (payload.argCount % 2 == 1) {
        // write the last byte
        writer.writeU8(argt);
    }

    // now, encode the actual argument values
    for (size_t i = 0; i < payload.argCount; i++) {
        auto& arg = payload.args[i];
        writer.writeU32(arg.rawValue);
    }

    // done!
}

std::optional<FireServerPayload> FireServerObject::decodePayload() {
    qn::HeapByteWriter writer;

    // could add other props if not enough space :p
    writer.writeU32(std::bit_cast<uint32_t>(m_item1Mode));
    writer.writeU32(std::bit_cast<uint32_t>(m_item2Mode));
    writer.writeU32(std::bit_cast<uint32_t>(m_targetItemMode));
    writer.writeU32(std::bit_cast<uint32_t>(m_resultType1));
    writer.writeU32(std::bit_cast<uint32_t>(m_resultType2));
    writer.writeU32(std::bit_cast<uint32_t>(m_roundType1));
    writer.writeU32(std::bit_cast<uint32_t>(m_roundType2));
    writer.writeU32(std::bit_cast<uint32_t>(m_signType1));
    writer.writeU32(std::bit_cast<uint32_t>(m_signType2));
    auto written = writer.written();

    // check the checksum (last byte)
    auto data = written.subspan(0, written.size() - 1);
    auto csum = written[written.size() - 1];

    auto expected = computeChecksum(data);
    if (csum != expected) {
        log::error("failed to validate checksum in FireServerObject args, expected {}, got {}", expected, csum);
        return std::nullopt;
    }

    auto res = ::globed::decodePayload(data);
    if (!res) {
        log::error("failed to decode FireServerObject args: {}", res.unwrapErr());
        return std::nullopt;
    }

    return res.unwrap();
}

void FireServerObject::encodePayload(const FireServerPayload& args) {
    qn::HeapByteWriter writer;
    ::globed::encodePayload(args, writer);

    // compute the checksum
    auto data = writer.written();
    auto csum = computeChecksum(data);
    writer.writeU8(csum);

    // write the data to the object

    data = writer.written();
    qn::ByteReader reader{data};

    auto locations = std::to_array<void*>({
        &m_item1Mode,
        &m_item2Mode,
        &m_targetItemMode,
        &m_resultType1,
        &m_resultType2,
        &m_roundType1,
        &m_roundType2,
        &m_signType1,
        &m_signType2
    });

    if (reader.remainingSize() > sizeof(uint32_t) * locations.size()) {
        log::error("Not enough data to fill all fields in FireServerObject! need >= {} bytes of space", reader.remainingSize());
        return;
    }

    for (auto locptr : locations) {
        uint32_t* loc = static_cast<uint32_t*>(locptr);

        if (reader.remainingSize() >= sizeof(uint32_t)) {
            *loc = *reader.readU32();
        } else if (reader.remainingSize() > 0) {
            reader.readBytes(reinterpret_cast<uint8_t*>(loc), reader.remainingSize()).unwrap();
        } else {
            // fill the rest with 0s
            *loc = 0;
        }
    }
}

}
