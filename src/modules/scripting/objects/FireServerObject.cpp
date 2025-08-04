#include "FireServerObject.hpp"

using namespace geode::prelude;

namespace globed {

FireServerObject::FireServerObject() {}

void FireServerObject::triggerObject(GJBaseGameLayer* p0, int p1, gd::vector<int> const* p2) {
    EffectGameObject::triggerObject(p0, p1, p2);
    log::debug("Fire emoji");
}

}
