#pragma once

#include <globed/core/data/Event.hpp>
#include <modules/scripting/objects/ExtendedObjectBase.hpp>
#include <qunet/buffers/ByteReader.hpp>
#include <qunet/buffers/HeapByteWriter.hpp>

namespace globed {

struct SpawnData {
    int groupId;
    std::optional<float> delay;
    float delayVariance;
    bool ordered;
    gd::vector<int> remaps{};
};

inline geode::Result<SpawnData> decodeSpawnData(qn::ByteReader &reader)
{
    SpawnData out{};

    uint8_t flagByte = READER_UNWRAP(reader.readU8());
    bool hasDelay = flagByte & (1 << 0);
    bool hasDelayVariance = hasDelay && flagByte & (1 << 1);
    bool isOrdered = flagByte & (1 << 2);
    bool hasRemaps = flagByte & (1 << 3);

    out.groupId = READER_UNWRAP(reader.readVarUint());

    if (hasDelay) {
        out.delay = READER_UNWRAP(reader.readF32());
    }

    if (hasDelayVariance) {
        out.delayVariance = READER_UNWRAP(reader.readF32());
    }

    out.ordered = isOrdered;

    if (hasRemaps) {
        auto pairCount = READER_UNWRAP(reader.readU8());
        // if (pairCount > 255) {
        //     return geode::Err("Too many remaps in spawn data");
        // }

        out.remaps.reserve(pairCount * 2);

        for (size_t i = 0; i < pairCount * 2; i++) {
            auto num = READER_UNWRAP(reader.readVarUint());
            out.remaps.push_back(num);
        }
    }

    return geode::Ok(out);
}

} // namespace globed
