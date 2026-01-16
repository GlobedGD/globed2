#include "ListenEventObject.hpp"
#include <modules/scripting/hooks/GJBaseGameLayer.hpp>

using namespace geode::prelude;

namespace globed {

ListenEventObject::ListenEventObject() {}

void ListenEventObject::triggerObject(GJBaseGameLayer *gjbgl, int p1, gd::vector<int> const *p2)
{
    auto bgl = SCBaseGameLayer::get(gjbgl);

    if (auto payload = this->decodePayload()) {
        bgl->addEventListener(*payload);
    } else {
        log::warn("Failed to decode ListenEventObject payload! {}", this);
    }
}

std::optional<ListenEventPayload> ListenEventObject::decodePayload()
{
    return ExtendedObjectBase::decodePayloadOpt<ListenEventPayload>(
        [&](qn::ByteReader &reader) -> Result<ListenEventPayload> {
            auto eventId = READER_UNWRAP(reader.readU16());
            auto groupId = READER_UNWRAP(reader.readI32());

            return Ok(ListenEventPayload{.eventId = eventId, .groupId = groupId});
        });
}

void ListenEventObject::encodePayload(const ListenEventPayload &payload)
{
    ExtendedObjectBase::encodePayload([&](qn::HeapByteWriter &writer) {
        writer.writeU16(payload.eventId);
        writer.writeI32(payload.groupId);
        return true;
    });
}

} // namespace globed