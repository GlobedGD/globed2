#include "editor_pause_layer.hpp"

#include "gjbasegamelayer.hpp"

using namespace geode::prelude;

void GlobedEditorPauseLayer::onSaveAndPlay(cocos2d::CCObject* sender) {
    EditorPauseLayer::onSaveAndPlay(sender);

    auto* pl = GlobedGJBGL::get();
    if (pl) {
        // make sure we remove listeners and stuff
        pl->onQuitActions();
    }
}