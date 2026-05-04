#pragma once

#include <dbuf/ByteReader.hpp>
#include <modules/scripting/objects/ExtendedObjectBase.hpp>

namespace globed {

struct MoveGroupData {
    int group;
    int center;
    float x, y, duration;
    bool absolute;
};

struct MoveAbsGroupData {
    int group;
    int center;
    float x, y;
};

inline geode::Result<MoveGroupData> decodeMoveGroupData(dbuf::ByteReader<>& reader) {
    MoveGroupData out{};

    out.absolute = GEODE_UNWRAP(reader.readBool());
    out.group = GEODE_UNWRAP(reader.readVarUint());

    if (out.absolute) {
        out.center = GEODE_UNWRAP(reader.readVarUint());
    }

    out.x = GEODE_UNWRAP(reader.readF32());
    out.y = GEODE_UNWRAP(reader.readF32());
    out.duration = GEODE_UNWRAP(reader.readF32());

    return geode::Ok(out);
}

inline geode::Result<MoveAbsGroupData> decodeMoveAbsGroupData(dbuf::ByteReader<>& reader) {
    MoveAbsGroupData out{};

    out.group = GEODE_UNWRAP(reader.readVarUint());
    out.center = GEODE_UNWRAP(reader.readVarUint());
    out.x = GEODE_UNWRAP(reader.readF32());
    out.y = GEODE_UNWRAP(reader.readF32());

    return geode::Ok(out);
}


}
