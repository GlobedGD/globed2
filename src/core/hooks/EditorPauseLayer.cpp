#include "GJBaseGameLayer.hpp"
#include <Geode/modify/EditorPauseLayer.hpp>

using namespace geode::prelude;

namespace globed {

struct GLOBED_DLL GLOBED_NOVTABLE HookedEditorPauseLayer : geode::Modify<HookedEditorPauseLayer, EditorPauseLayer> {
    $override
    void onSaveAndPlay(CCObject* sender) {
        EditorPauseLayer::onSaveAndPlay(sender);

        if (auto pl = GlobedGJBGL::get()) {
            pl->onQuit();
        }
    }
};

}
