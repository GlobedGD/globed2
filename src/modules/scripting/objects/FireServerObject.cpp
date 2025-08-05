#include "FireServerObject.hpp"
#include <globed/util/assert.hpp>
#include <globed/util/format.hpp>
#include <core/net/NetworkManagerImpl.hpp>

#include <qunet/buffers/HeapByteWriter.hpp> // TODO: ByteWriter instead when implemented
#include <qunet/buffers/ByteReader.hpp>

using namespace geode::prelude;

namespace globed {

FireServerObject::FireServerObject() {}

void FireServerObject::triggerObject(GJBaseGameLayer* gjbgl, int p1, gd::vector<int> const* p2) {
    // EffectGameObject::triggerObject(p0, p1, p2);
    log::debug("firing!");

    auto res = this->decodePayload();
    if (!res) {
        log::warn("Failed to decode FireServerObject payload! {}", this);
        return;
    }

    auto& payload = *res;

    qn::HeapByteWriter writer;
    writer.writeU8(payload.argCount);

    // Encode the argument types, MSB is for first argument and LSB is for the last argument.
    // one bit per argument (max 8), bit 1 means float, bit 0 means int.

    uint8_t typeByte = 0;
    uint8_t shift = 7;

    for (size_t i = 0; i < payload.argCount; i++) {
        auto& arg = payload.args[i];

        if (arg.type == FireServerArgType::Timer) {
            // this is a float
            typeByte |= (1 << shift);
        } else if (arg.type == FireServerArgType::Item || arg.type == FireServerArgType::Static) {
            // this is an int, do nothing
        } else {
            // this should never happen
            GLOBED_ASSERT(false && "Invalid FireServerArgType in payload");
            std::unreachable();
        }

        shift--;
    }

    writer.writeU8(typeByte);

    // encode the values

    for (size_t i = 0; i < payload.argCount; i++) {
        auto& arg = payload.args[i];

        if (arg.type == FireServerArgType::Static) {
            // treat value as a static int value
            writer.writeI32(arg.value);
        } else if (arg.type == FireServerArgType::Item) {
            // treat value as an item ID
            auto value = (int) gjbgl->getItemValue(1, arg.value);
            writer.writeI32(value);
        } else if (arg.type == FireServerArgType::Timer) {
            // treat value as a timer ID
            float value = gjbgl->getItemValue(2, arg.value);
            writer.writeF32(value);
        }
    }

    // send off the event
    NetworkManagerImpl::get().queueGameEvent(Event {
        .type = payload.eventId,
        .data = std::move(writer).intoVector(),
    });
}

static uint8_t computeChecksum(std::span<const uint8_t> data) {
    uint32_t sum = 0;
    for (auto byte : data) {
        sum += byte;
    }

    return ~sum & 0xff;
}

static Result<FireServerPayload, qn::ByteReaderError> decodePayload(qn::ByteReader& reader) {
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
        out.args[i].rawValue = GEODE_UNWRAP(reader.readVarUint());
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
        writer.writeVarUint(arg.rawValue).unwrap();
    }

    // done!
}

std::optional<FireServerPayload> FireServerObject::decodePayload() {
    qn::HeapByteWriter writer;

    // could add other props if not enough space :p
    writer.writeU32(std::bit_cast<uint32_t>(m_item1Mode));
    writer.writeU32(std::bit_cast<uint32_t>(m_item2Mode));
    writer.writeU32(std::bit_cast<uint32_t>(m_resultType1));
    writer.writeU32(std::bit_cast<uint32_t>(m_resultType2));
    writer.writeU32(std::bit_cast<uint32_t>(m_roundType1));
    writer.writeU32(std::bit_cast<uint32_t>(m_roundType2));
    writer.writeU32(std::bit_cast<uint32_t>(m_signType1));
    writer.writeU32(std::bit_cast<uint32_t>(m_signType2));
    auto written = writer.written();

    log::debug("Decoding payload: {}", hexEncode(written.data(), written.size()));

    qn::ByteReader reader{written};

    auto res = ::globed::decodePayload(reader);
    if (!res) {
        log::error("failed to decode FireServerObject args: {}", res.unwrapErr().message());
        return std::nullopt;
    }

    // validate the payload
    auto payload = res.unwrap();
    for (size_t i = 0; i < payload.argCount; i++) {
        auto& arg = payload.args[i];

        if (arg.type == FireServerArgType::None) {
            log::error("FireServerObject has an argument with type None at index {}", i);
            return std::nullopt;
        }
    }

    // check the checksum (last byte)
    auto csumres = reader.readU8();
    if (!csumres) {
        log::error("failed to read checksum from FireServerObject args: {}", csumres.unwrapErr().message());
        return std::nullopt;
    }

    auto csum = csumres.unwrap();
    auto data = written.subspan(0, reader.position() - 1);

    auto expected = computeChecksum(data);
    if (csum != expected) {
        log::error("failed to validate checksum in FireServerObject args, expected {}, got {}", expected, csum);
        return std::nullopt;
    }

    return payload;
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

    log::debug("Encoding payload: {}", hexEncode(data.data(), data.size()));

    auto locations = std::to_array<void*>({
        &m_item1Mode,
        &m_item2Mode,
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

// a0a1a2a3a4a5a6a7a8a9aaabacadaeafb0b1b2b3b4b5b6b7b8b9babbbcbdbeb9
// a0a1a2a3a4a5a6a7a8a9aaabacadaeafb0b1b2b3b4b5b6b7b8b9babbbcbdbeb9
// a0a1a2a3a4a5a6a7a8a9aaabacadaeafb0b1b2b3b4b5b6b7b8b9babbbcbdbeb9