#include "FireServerObject.hpp"
#include <globed/util/assert.hpp>
#include <core/net/NetworkManagerImpl.hpp>

using namespace geode::prelude;

namespace globed {

FireServerObject::FireServerObject() {}

void FireServerObject::triggerObject(GJBaseGameLayer* gjbgl, int p1, gd::vector<int> const* p2) {
    auto res = this->decodePayload();
    if (!res) {
        log::warn("Failed to decode FireServerObject payload! {}", this);
        return;
    }

    auto& payload = *res;

    ScriptedEvent ev{};
    ev.type = payload.eventId;

    // encode the values

    for (size_t i = 0; i < payload.argCount; i++) {
        auto& arg = payload.args[i];

        if (arg.type == FireServerArgType::Static) {
            // treat value as a static int value
            ev.args.push_back(arg.value);
        } else if (arg.type == FireServerArgType::Item) {
            // treat value as an item ID
            auto value = (int) gjbgl->getItemValue(1, arg.value);
            ev.args.push_back(value);
        } else if (arg.type == FireServerArgType::Timer) {
            // treat value as a timer ID
            float value = gjbgl->getItemValue(2, arg.value);
            ev.args.push_back(value);
        }
    }

    // send off the event
    NetworkManagerImpl::get().queueGameEvent(std::move(ev));
}

static Result<FireServerPayload, qn::ByteReaderError> decodePayload(qn::ByteReader& reader) {
    FireServerPayload out{};
    out.eventId = GEODE_UNWRAP(reader.readU16());
    out.argCount = GEODE_UNWRAP(reader.readU8());

    uint8_t argt = 0;

    for (size_t i = 0; i < out.argCount; i++) {
        auto& outArg = out.args[i];

        if (i % 2 == 0) {
            argt = GEODE_UNWRAP(reader.readU8());
            outArg.type = static_cast<FireServerArgType>(argt >> 4);
        } else {
            outArg.type = static_cast<FireServerArgType>(argt & 0xf);
        }
    }

    for (size_t i = 0; i < out.argCount; i++) {
        out.args[i].rawValue = GEODE_UNWRAP(reader.readVarUint());
    }

    return Ok(out);
}

static void encodePayload(const FireServerPayload& payload, qn::HeapByteWriter& writer) {
    writer.writeU16(payload.eventId);
    writer.writeU8(payload.argCount);

    // encode the argument types, 1 byte holds two 4-bit values

    uint8_t argt = 0;

    for (size_t i = 0; i < payload.argCount; i++) {
        auto& arg = payload.args[i];

        if (i % 2 == 0) {
            argt = static_cast<uint8_t>(arg.type) << 4;
        } else {
            argt |= static_cast<uint8_t>(arg.type);
            writer.writeU8(argt);
        }
    }

    if (payload.argCount % 2 == 1) {
        // write the last byte
        writer.writeU8(argt);
    }

    // now, encode the actual argument values
    for (size_t i = 0; i < payload.argCount; i++) {
        auto& arg = payload.args[i];
        writer.writeVarUint(arg.rawValue).unwrap();
    }

    // done!
}

std::optional<FireServerPayload> FireServerObject::decodePayload() {
    return ExtendedObjectBase::decodePayloadOpt<FireServerPayload>([&](qn::ByteReader& reader) -> Result<FireServerPayload> {
        auto payload = READER_UNWRAP(::globed::decodePayload(reader));

        // validate the payload
        if (payload.eventId >= EVENT_GLOBED_BASE) {
            return Err("FireServerObject has an invalid event ID: {}", payload.eventId);
        }

        for (size_t i = 0; i < payload.argCount; i++) {
            auto& arg = payload.args[i];

            if (arg.type == FireServerArgType::None) {
                return Err("FireServerObject has an argument with type None at index {}", i);
            }
        }

        return Ok(std::move(payload));
    });
}

void FireServerObject::encodePayload(const FireServerPayload& args) {
    ExtendedObjectBase::encodePayload([&](qn::HeapByteWriter& writer) {
        ::globed::encodePayload(args, writer);
        return true;
    });
}

}
