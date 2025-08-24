#pragma once

#include <qunet/buffers/ByteReader.hpp>
#include <modules/scripting/objects/ExtendedObjectBase.hpp>

namespace globed {

struct FollowPlayerData {
    int player;
    int group;
    bool enable;
};

inline geode::Result<FollowPlayerData> decodeFollowPlayerData(qn::ByteReader& reader) {
    FollowPlayerData out{};

    uint16_t val = READER_UNWRAP(reader.readU16());

    // clear top bit, it indicates enabled state
    out.group = (int)(val & ~(1 << 15));
    out.enable = val & (1 << 15);
    out.player = READER_UNWRAP(reader.readI32());

    return geode::Ok(out);
}

}
