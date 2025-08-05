#pragma once

#include <Geode/Geode.hpp>

namespace globed {

enum class FireServerArgType {
    None = 0, Item = 1, Timer = 2, Static = 3,
};

struct FireServerArg {
    FireServerArgType type;

    union {
        int itemId;
        int value;
        float timer;

        uint32_t rawValue;
    };
};

struct FireServerPayload {
    uint16_t eventId;
    std::array<FireServerArg, 8> args;
    size_t argCount;
};

class FireServerObject : public ItemTriggerGameObject {
public:
    FireServerObject();

    void triggerObject(GJBaseGameLayer* p0, int p1, gd::vector<int> const* p2) override;

    std::optional<FireServerPayload> decodePayload();
    void encodePayload(const FireServerPayload& args);

private:
};

}
