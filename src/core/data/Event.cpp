#include <globed/core/data/Event.hpp>

using namespace qn;
using namespace geode::prelude;

#define READER_UNWRAP(...) GEODE_UNWRAP((__VA_ARGS__).mapErr([&](auto&& err) { return err.message(); }));

namespace globed {

// In n out events

Result<CounterChangeEvent> CounterChangeEvent::decode(ByteReader& reader) {
    uint64_t rawData = READER_UNWRAP(reader.readU64());

    uint8_t rawType = (rawData >> 56) & 0xff;
    uint32_t itemId = (rawData >> 32) & 0x00fffff;
    uint32_t value = static_cast<uint32_t>(rawData & 0xffffffffULL);

    return Ok(CounterChangeEvent {
        rawType, itemId, value
    });
}

Result<> CounterChangeEvent::encode(HeapByteWriter& writer) {
    writer.writeU16(EVENT_COUNTER_CHANGE);

    uint64_t rawData =
        (static_cast<uint64_t>(rawType) << 56)
        | ((static_cast<uint64_t>(itemId) & 0x00ffffffull) << 32);

    rawData |= rawValue;

    writer.writeU64(rawData);

    return Ok();
}

Result<TwoPlayerLinkRequestEvent> TwoPlayerLinkRequestEvent::decode(ByteReader& reader) {
    int id = READER_UNWRAP(reader.readI32());
    bool player1 = READER_UNWRAP(reader.readBool());

    return Ok(TwoPlayerLinkRequestEvent { id, player1 });
}

Result<> TwoPlayerLinkRequestEvent::encode(HeapByteWriter& writer) {
    writer.writeU16(EVENT_2P_LINK_REQUEST);
    writer.writeI32(playerId);
    writer.writeBool(player1);
    return Ok();
}

Result<TwoPlayerUnlinkEvent> TwoPlayerUnlinkEvent::decode(ByteReader& reader) {
    int id = READER_UNWRAP(reader.readI32());
    return Ok(TwoPlayerUnlinkEvent { id });
}

Result<> TwoPlayerUnlinkEvent::encode(HeapByteWriter& writer) {
    writer.writeU16(EVENT_2P_UNLINK);
    writer.writeI32(playerId);
    return Ok();
}

// In events

Result<SpawnGroupEvent> SpawnGroupEvent::decode(qn::ByteReader& reader) {
    return Ok(GEODE_UNWRAP(decodeSpawnData(reader)));
}

Result<SetItemEvent> SetItemEvent::decode(qn::ByteReader& reader) {
    return Ok(GEODE_UNWRAP(decodeSetItemData(reader)));
}

Result<MoveGroupEvent> MoveGroupEvent::decode(qn::ByteReader& reader) {
    return Ok(GEODE_UNWRAP(decodeMoveGroupData(reader)));
}

Result<MoveGroupAbsoluteEvent> MoveGroupAbsoluteEvent::decode(qn::ByteReader& reader) {
    return Ok(GEODE_UNWRAP(decodeMoveAbsGroupData(reader)));
}

Result<FollowPlayerEvent> FollowPlayerEvent::decode(qn::ByteReader& reader) {
    return Ok(GEODE_UNWRAP(decodeFollowPlayerData(reader)));
}

Result<FollowRotationEvent> FollowRotationEvent::decode(qn::ByteReader& reader) {
    return Ok(GEODE_UNWRAP(decodeFollowRotationData(reader)));
}

Result<ActivePlayerSwitchEvent> ActivePlayerSwitchEvent::decode(qn::ByteReader& reader) {
    auto playerId = READER_UNWRAP(reader.readI32());
    auto type = READER_UNWRAP(reader.readU8());

    return Ok(ActivePlayerSwitchEvent { playerId, type });
}

Result<> ActivePlayerSwitchEvent::encode(qn::HeapByteWriter& writer) {
    writer.writeU16(EVENT_ACTIVE_PLAYER_SWITCH);
    writer.writeI32(playerId);
    writer.writeU8(type);

    return Ok();
}

Result<InEvent> InEvent::decode(ByteReader& reader) {
    auto type = READER_UNWRAP(reader.readU16());

#define MAP_TO(ty, cls) case ty: return Ok(GEODE_UNWRAP(cls::decode(reader)))
    switch (type) {
        MAP_TO(EVENT_COUNTER_CHANGE, CounterChangeEvent);
        MAP_TO(EVENT_SCR_SPAWN_GROUP, SpawnGroupEvent);
        MAP_TO(EVENT_SCR_SET_ITEM, SetItemEvent);
        MAP_TO(EVENT_SCR_MOVE_GROUP, MoveGroupEvent);
        MAP_TO(EVENT_SCR_MOVE_GROUP_ABSOLUTE, MoveGroupAbsoluteEvent);
        MAP_TO(EVENT_SCR_FOLLOW_PLAYER, FollowPlayerEvent);
        MAP_TO(EVENT_SCR_FOLLOW_ROTATION, FollowRotationEvent);
        MAP_TO(EVENT_2P_LINK_REQUEST, TwoPlayerLinkRequestEvent);
        MAP_TO(EVENT_2P_UNLINK, TwoPlayerUnlinkEvent);
        MAP_TO(EVENT_ACTIVE_PLAYER_SWITCH, ActivePlayerSwitchEvent);
        default: break;
    }
#undef MAP_TO

#ifdef GLOBED_DEBUG
    geode::log::debug("Received unknown event type: {}", type);
#endif

    size_t remSize = reader.remainingSize();
    std::vector<uint8_t> rawData;
    rawData.resize(remSize);
    reader.readBytes(rawData.data(), remSize).unwrap();

    return Ok(InEvent {
        UnknownEvent { type, std::move(rawData) }
    });
}

// Out events

Result<> ScriptedEvent::encode(HeapByteWriter& writer) {
    writer.writeU16(type);

    writer.writeU8(args.size());

    // Encode the argument types, MSB is for first argument and LSB is for the last argument.
    // one bit per argument (max 8), bit 1 means float, bit 0 means int.

    uint8_t typeByte = 0;
    uint8_t shift = 7;

    for (auto& arg : args) {
        bool isFloat = std::holds_alternative<float>(arg);

        if (isFloat) {
            // this is a float
            typeByte |= (1 << shift);
        } else {
            // this is an int, do nothing
        }

        shift--;
    }

    writer.writeU8(typeByte);

    // encode the values

    for (size_t i = 0; i < args.size(); i++) {
        auto& arg = args[i];
        bool isFloat = std::holds_alternative<float>(arg);

        if (isFloat) {
            writer.writeF32(std::get<float>(arg));
        } else {
            writer.writeI32(std::get<int>(arg));
        }
    }

    return Ok();
}

Result<> RequestScriptLogsEvent::encode(HeapByteWriter& writer) {
    writer.writeU16(EVENT_SCR_REQUEST_SCRIPT_LOGS);
    return Ok();
}

Result<> UnknownEvent::encode(HeapByteWriter& writer) {
    writer.writeU16(type);
    writer.writeBytes(rawData.data(), rawData.size());
    return Ok();
}

Result<> OutEvent::encode(HeapByteWriter& writer) {
    return std::visit([&](auto&& obj) {
        return obj.encode(writer);
    }, m_kind);
}

}
