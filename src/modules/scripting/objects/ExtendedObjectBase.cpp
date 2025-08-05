#include "ExtendedObjectBase.hpp"
#include <globed/util/format.hpp>

using namespace geode::prelude;

namespace globed {

ExtendedObjectBase::ExtendedObjectBase() {}

void ExtendedObjectBase::encodePayload(std::function<bool(qn::HeapByteWriter&)>&& writefn) {
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

}
