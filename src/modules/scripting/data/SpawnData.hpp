#pragma once

#include <dbuf/ByteReader.hpp>
#include <dbuf/ByteWriter.hpp>
#include <globed/core/data/Event.hpp>
#include <modules/scripting/objects/ExtendedObjectBase.hpp>

namespace globed {

struct SpawnData {
    int groupId;
    std::optional<float> delay;
    float delayVariance;
    bool ordered;
    gd::vector<int> remaps = gd::vector<int>{};
};

inline geode::Result<SpawnData> decodeSpawnData(dbuf::ByteReader<>& reader) {
    SpawnData out{};

    uint8_t flagByte = GEODE_UNWRAP(reader.readU8());
    bool hasDelay = flagByte & (1 << 0);
    bool hasDelayVariance = hasDelay && flagByte & (1 << 1);
    bool isOrdered = flagByte & (1 << 2);
    bool hasRemaps = flagByte & (1 << 3);

    out.groupId = GEODE_UNWRAP(reader.readVarUint());

    if (hasDelay) {
        out.delay = GEODE_UNWRAP(reader.readF32());
    }

    if (hasDelayVariance) {
        out.delayVariance = GEODE_UNWRAP(reader.readF32());
    }

    out.ordered = isOrdered;

    if (hasRemaps) {
        size_t pairCount = GEODE_UNWRAP(reader.readU8());
        // if (pairCount > 255) {
        //     return geode::Err("Too many remaps in spawn data");
        // }

        out.remaps.reserve(pairCount * 2);

        for (size_t i = 0; i < pairCount * 2; i++) {
            auto num = GEODE_UNWRAP(reader.readVarUint());
            out.remaps.push_back(num);
        }
    }

    return geode::Ok(std::move(out));
}

}
