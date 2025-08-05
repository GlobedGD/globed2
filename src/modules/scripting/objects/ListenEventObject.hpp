#pragma once

#include "ExtendedObjectBase.hpp"

namespace globed {

struct ListenEventPayload {
    uint16_t eventId;
    int groupId;
};

class ListenEventObject : public ExtendedObjectBase {
public:
    ListenEventObject();

    void triggerObject(GJBaseGameLayer* p0, int p1, gd::vector<int> const* p2) override;

    std::optional<ListenEventPayload> decodePayload();
    void encodePayload(const ListenEventPayload& payload);
private:
};

}
