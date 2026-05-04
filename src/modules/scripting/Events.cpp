#include "Events.hpp"
#include <dbuf/ByteReader.hpp>
#include <dbuf/ByteWriter.hpp>

using namespace geode::prelude;

namespace globed {

Result<SpawnGroupEvent> SpawnGroupEvent::decode(std::span<const uint8_t> data) {
    dbuf::ByteReader reader(data);
    return Ok(SpawnGroupEvent {
        GEODE_UNWRAP(decodeSpawnData(reader))
    });
}

Result<SetItemEvent> SetItemEvent::decode(std::span<const uint8_t> data) {
    dbuf::ByteReader reader(data);
    return Ok(SetItemEvent {
        GEODE_UNWRAP(decodeSetItemData(reader))
    });
}

Result<MoveGroupEvent> MoveGroupEvent::decode(std::span<const uint8_t> data) {
    dbuf::ByteReader reader(data);
    return Ok(MoveGroupEvent {
        GEODE_UNWRAP(decodeMoveGroupData(reader))
    });
}

Result<FollowPlayerEvent> FollowPlayerEvent::decode(std::span<const uint8_t> data) {
    dbuf::ByteReader reader(data);
    return Ok(FollowPlayerEvent {
        GEODE_UNWRAP(decodeFollowPlayerData(reader))
    });
}

Result<FollowAbsoluteEvent> FollowAbsoluteEvent::decode(std::span<const uint8_t> data) {
    dbuf::ByteReader reader(data);
    return Ok(FollowAbsoluteEvent {
        GEODE_UNWRAP(decodeFollowAbsoluteData(reader))
    });
}

Result<FollowRotationEvent> FollowRotationEvent::decode(std::span<const uint8_t> data) {
    dbuf::ByteReader reader(data);
    return Ok(FollowRotationEvent {
        GEODE_UNWRAP(decodeFollowRotationData(reader))
    });
}

std::vector<uint8_t> ScriptedEvent::encode() const {
    dbuf::ByteWriter<> writer;

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

    return std::move(writer).intoInner();
}

}