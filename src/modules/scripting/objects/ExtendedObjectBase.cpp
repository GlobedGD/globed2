#include "ExtendedObjectBase.hpp"
#include <globed/util/format.hpp>

using namespace geode::prelude;

namespace globed {

ExtendedObjectBase::ExtendedObjectBase() {}

void ExtendedObjectBase::encodePayload(std23::function_ref<bool(qn::HeapByteWriter&)>&& writefn) {
    qn::HeapByteWriter writer;
    writefn(writer);

    // compute the checksum
    auto data = writer.written();
    auto csum = this->computeChecksum(data);
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

qn::ByteReader ExtendedObjectBase::_decodePayloadPre(qn::ArrayByteWriter<64>& writer) {
    // could add other props if not enough space :p
    (void) writer.writeU32(std::bit_cast<uint32_t>(m_item1Mode));
    (void) writer.writeU32(std::bit_cast<uint32_t>(m_item2Mode));
    (void) writer.writeU32(std::bit_cast<uint32_t>(m_resultType1));
    (void) writer.writeU32(std::bit_cast<uint32_t>(m_resultType2));
    (void) writer.writeU32(std::bit_cast<uint32_t>(m_roundType1));
    (void) writer.writeU32(std::bit_cast<uint32_t>(m_roundType2));
    (void) writer.writeU32(std::bit_cast<uint32_t>(m_signType1));
    (void) writer.writeU32(std::bit_cast<uint32_t>(m_signType2));
    auto written = writer.written();

    log::debug("Decoding payload: {}", hexEncode(written.data(), written.size()));

    return qn::ByteReader{written};
}

Result<> ExtendedObjectBase::_decodePayloadPost(qn::ArrayByteWriter<64>& writer, qn::ByteReader& reader) {
    // check the checksum (last byte)
    auto csumres = reader.readU8();
    if (!csumres) {
        return Err("Failed to read checksum from script object data (for {}): {}", typeid(*this).name(), csumres.unwrapErr().message());
    }

    auto csum = csumres.unwrap();
    auto data = writer.written().subspan(0, reader.position() - 1);

    auto expected = this->computeChecksum(data);
    if (csum != expected) {
        return Err("Failed to validate checksum in script object data (for {}), expected {}, got {}", typeid(*this).name(), expected, csum);
    }

    return Ok();
}

}
