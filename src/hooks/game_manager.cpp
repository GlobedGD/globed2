#include "game_manager.hpp"

#include "gjgamelevel.hpp"

#include <util/debug.hpp>

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
            return m_fields->iconCache.at(iconType).at(iconId);
        }
    }

    auto* gm = GameManager::get();
    auto* textureCache = CCTextureCache::sharedTextureCache();
    auto* sfCache = CCSpriteFrameCache::sharedSpriteFrameCache();
    CCTexture2D* texture = nullptr;

    std::string sheetName = gm->sheetNameForIcon(iconId, iconType);
    if (!sheetName.empty()) {
        int key = gm->keyForIcon(iconId, iconType);

        if (gm->m_loadIcon[key] < 1) {
            texture = textureCache->addImage((sheetName + ".png").c_str(), false);
            sfCache->addSpriteFramesWithFile((sheetName + ".plist").c_str());
        } else {
            texture = textureCache->textureForKey((sheetName + ".png").c_str());
        }

        auto& thatMap = gm->m_loadIcon2[iconRequestId];
        auto existingIcon = thatMap[iconType];
        if (existingIcon != iconId) {
            gm->m_loadIcon[key]++;
            if (existingIcon > 0) {
                gm->unloadIcon(existingIcon, iconType, iconRequestId);
            }

            gm->m_loadIcon2[iconRequestId][iconType] = iconId;
        }
    }

    if (texture) {
        m_fields->iconCache[iconType][iconId] = texture;
    }

    return texture;
}