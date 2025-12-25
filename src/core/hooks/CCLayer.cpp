#include "GJBaseGameLayer.hpp"

#include <Geode/modify/CCLayer.hpp>

using namespace geode::prelude;

namespace globed {

struct GLOBED_MODIFY_ATTR HookedCCLayer : Modify<HookedCCLayer, CCLayer> {
    // Unfortunately, vmt hook manager is still broken, so we have to hook CCLayer::onEnter :(
    $override
    void onEnter() {
        auto* gjbgl = GlobedGJBGL::get();
        void* self = this;

        if (gjbgl && gjbgl == self && !gjbgl->isEditor() && gjbgl->active()) {
            gjbgl->onEnterHook();
        } else {
            CCLayer::onEnter();
        }
    }
};

}