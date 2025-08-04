#pragma once

#include <Geode/Geode.hpp>

namespace globed {

class FireServerObject : public ItemTriggerGameObject {
public:
    FireServerObject();
    void triggerObject(GJBaseGameLayer* p0, int p1, gd::vector<int> const* p2) override;

private:
};

}
