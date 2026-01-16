#pragma once

#include <modules/scripting/objects/ExtendedObjectBase.hpp>
#include <qunet/buffers/ByteReader.hpp>

namespace globed {

struct SetItemData {
    int itemId;
    int value;
};

inline geode::Result<SetItemData> decodeSetItemData(qn::ByteReader &reader)
{
    SetItemData out{};

    out.itemId = READER_UNWRAP(reader.readVarUint());
    out.value = READER_UNWRAP(reader.readVarInt());

    return geode::Ok(out);
}

} // namespace globed
