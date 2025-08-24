#pragma once

#include <qunet/buffers/ByteReader.hpp>
#include <modules/scripting/objects/ExtendedObjectBase.hpp>

namespace globed {

struct MoveGroupData {
    int group;
    float x, y;
};

struct MoveAbsGroupData {
    int group;
    int center;
    float x, y;
};

inline geode::Result<MoveGroupData> decodeMoveGroupData(qn::ByteReader& reader) {
    MoveGroupData out{};

    out.group = READER_UNWRAP(reader.readVarUint());
    out.x = READER_UNWRAP(reader.readF32());
    out.y = READER_UNWRAP(reader.readF32());

    return geode::Ok(out);
}

inline geode::Result<MoveAbsGroupData> decodeMoveAbsGroupData(qn::ByteReader& reader) {
    MoveAbsGroupData out{};

    out.group = READER_UNWRAP(reader.readVarUint());
    out.center = READER_UNWRAP(reader.readVarUint());
    out.x = READER_UNWRAP(reader.readF32());
    out.y = READER_UNWRAP(reader.readF32());

    return geode::Ok(out);
}


}
