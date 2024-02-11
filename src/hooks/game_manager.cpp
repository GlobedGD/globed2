#include "game_manager.hpp"

#include "gjgamelevel.hpp"

#include <managers/settings.hpp>
#include <util/debug.hpp>
#include <util/ui.hpp>

using namespace geode::prelude;

void HookedGameManager::returnToLastScene(GJGameLevel* level) {
    auto hooklevel = static_cast<HookedGJGameLevel*>(level);
    if (hooklevel->m_fields->shouldTransitionWithPopScene) {
        CCDirector::get()->popSceneWithTransition(0.5f, PopTransition::kPopTransitionFade);
    } else {
        GameManager::returnToLastScene(level);
    }
}

CCTexture2D* HookedGameManager::loadIcon(int iconId, int iconType, int iconRequestId) {
    if (m_fields->iconCache.contains(iconType)) {
        if (m_fields->iconCache.at(iconType).contains(iconId)) {
            auto* tex = *m_fields->iconCache.at(iconType).at(iconId);
            return tex;
        }
    }

    CCTexture2D* texture = GameManager::loadIcon(iconId, iconType, iconRequestId);

    if (texture) {
        m_fields->iconCache[iconType][iconId] = texture;
    }

    return texture;
}

void HookedGameManager::reloadAllStep2() {
    m_fields->iconCache.clear();
    GameManager::reloadAllStep2();
}
