#include "game_manager.hpp"

#include "gjgamelevel.hpp"

using namespace geode::prelude;

void HookedGameManager::returnToLastScene(GJGameLevel* level) {
    auto hooklevel = static_cast<HookedGJGameLevel*>(level);
    if (hooklevel->m_fields->shouldTransitionWithPopScene) {
        CCDirector::get()->popSceneWithTransition(0.5f, PopTransition::kPopTransitionFade);
    } else {
        GameManager::returnToLastScene(level);
    }
}