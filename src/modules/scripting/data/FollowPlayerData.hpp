#pragma once

#include <qunet/buffers/ByteReader.hpp>
#include <modules/scripting/objects/ExtendedObjectBase.hpp>

namespace globed {

struct FollowPlayerData {
    int player;
    int group;
    bool enable;
};

struct FollowRotationData {
    int player;
    int group;
    int center;
    bool enable;
};

inline geode::Result<FollowPlayerData> decodeFollowPlayerData(qn::ByteReader& reader) {
    FollowPlayerData out{};

    uint16_t val = READER_UNWRAP(reader.readU16());

    out.enable = val & (1 << 15);
    out.group = (int)(val & 0b01111111'11111111);
    out.player = READER_UNWRAP(reader.readI32());

    return geode::Ok(out);
}

inline geode::Result<FollowRotationData> decodeFollowRotationData(qn::ByteReader& reader) {
    FollowRotationData out{};

    uint16_t group = READER_UNWRAP(reader.readU16());
    uint16_t center = READER_UNWRAP(reader.readU16());

    out.enable = group & (1 << 15);
    out.group = (int)(group & 0b01111111'11111111);
    out.center = center;
    out.player = READER_UNWRAP(reader.readI32());

    return geode::Ok(out);
}

}
