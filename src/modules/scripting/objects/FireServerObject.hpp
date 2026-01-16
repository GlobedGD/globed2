#pragma once

#include "ExtendedObjectBase.hpp"

namespace globed {

enum class FireServerArgType {
    None = 0,
    Item = 1,
    Timer = 2,
    Static = 3,
};

struct FireServerArg {
    FireServerArgType type;

    union {
        int value;

        uint32_t rawValue;
    };
};

struct FireServerPayload {
    uint16_t eventId;
    uint8_t argCount;
    std::array<FireServerArg, 8> args;
};

class FireServerObject : public ExtendedObjectBase {
public:
    FireServerObject();

    void triggerObject(GJBaseGameLayer *p0, int p1, gd::vector<int> const *p2) override;

    std::optional<FireServerPayload> decodePayload();
    void encodePayload(const FireServerPayload &args);

private:
};

} // namespace globed
