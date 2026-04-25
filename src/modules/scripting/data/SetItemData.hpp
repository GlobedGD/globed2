#pragma once

#include <dbuf/ByteReader.hpp>
#include <modules/scripting/objects/ExtendedObjectBase.hpp>

namespace globed {

struct SetItemData {
    int itemId;
    int value;
};

inline geode::Result<SetItemData> decodeSetItemData(dbuf::ByteReader<>& reader) {
    SetItemData out{};

    out.itemId = GEODE_UNWRAP(reader.readVarUint());
    out.value = GEODE_UNWRAP(reader.readVarInt());

    return geode::Ok(out);
}

}
