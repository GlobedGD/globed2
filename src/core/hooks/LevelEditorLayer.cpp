#include "GJBaseGameLayer.hpp"
#include <Geode/modify/LevelEditorLayer.hpp>

using namespace geode::prelude;

namespace globed {

struct GLOBED_MODIFY_ATTR HookedEditorLayer : geode::Modify<HookedEditorLayer, LevelEditorLayer> {
    $override
    bool init(GJGameLevel* level, bool a) {
        auto gjbgl = GlobedGJBGL::get(this);
        gjbgl->setupPreInit(level, true);

        if (!LevelEditorLayer::init(level, a)) return false;

        gjbgl->setupPostInit();

        return true;
    }
};

}
