#include "game_manager.hpp"

#include "gjgamelevel.hpp"

#include <managers/settings.hpp>
#include <util/debug.hpp>
#include <util/ui.hpp>
#include <util/cocos.hpp>

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
    CCTexture2D* texture = this->getCachedIcon(iconId, iconType);

    if (texture) {
        auto sheetName = this->sheetNameForIcon(iconId, iconType);
        this->verifyCachedData();

        auto fullpath = util::cocos::fullPathForFilename(fmt::format("{}.png", sheetName), m_fields->cachedTexNameSuffix, m_fields->cachedSearchPathIdx);

        if (!CCSpriteFrameCache::get()->m_pSpriteFrames->objectForKey(fullpath)) {
            log::warn("player texture exists but sprite frame is invalid! id: {}, icon type: {}", iconId, iconType);
            // this->m_fields->iconCache[iconType].erase(iconId);
            // texture = nullptr;
        }
    }

    if (texture) {
        return texture;
    }

    texture = GameManager::loadIcon(iconId, iconType, iconRequestId);

    if (texture) {
        m_fields->iconCache[iconType][iconId] = texture;
    }

    return texture;
}

void HookedGameManager::reloadAllStep2() {
    m_fields->iconCache.clear();
    m_fields->loadedFrames.clear();
    m_fields->cachedTexNameSuffix.clear();
    m_fields->cachedSearchPathIdx = -1;

    this->setAssetsPreloaded(false);
    GameManager::reloadAllStep2();
}

void HookedGameManager::loadIconsBatched(int iconType, int startId, int endId) {
    auto ranges = std::vector<BatchedIconRange>({BatchedIconRange {
        .iconType = iconType,
        .startId = startId,
        .endId = endId
    }});

    this->loadIconsBatched(ranges);
}

void HookedGameManager::loadIconsBatched(const std::vector<BatchedIconRange>& ranges) {
    std::vector<std::string> toLoad;
    std::unordered_map<int, std::unordered_map<int, size_t>> toLoadMap;

    for (const auto& range : ranges) {
        for (int id = range.startId; id <= range.endId; id++) {
            auto sheetName = this->sheetNameForIcon(id, range.iconType);
            if (sheetName.empty()) continue;

            toLoad.push_back(sheetName);
            toLoadMap[range.iconType][id] = toLoad.size() - 1;
        }
    }

    util::cocos::loadAssetsParallel(toLoad);

    auto* tc = CCTextureCache::sharedTextureCache();

    this->verifyCachedData();

    for (const auto& [iconType, map] : toLoadMap) {
        for (const auto& [iconId, idx] : map) {
            const auto& sheetName = toLoad[idx];

            auto fullpath = util::cocos::fullPathForFilename(fmt::format("{}.png", sheetName), m_fields->cachedTexNameSuffix, m_fields->cachedSearchPathIdx);
            if (fullpath.empty()) {
                log::warn("path empty: {}", sheetName);
                continue;
            }

            auto* tex = static_cast<CCTexture2D*>(tc->m_pTextures->objectForKey(fullpath));

            if (!tex) {
                log::warn("icon failed to preload: type {}, id {}", iconType, iconId);
                continue;
            }

            auto* existing = this->getCachedIcon(iconId, iconType);
            if (existing && existing != tex) {
                log::warn("icon already exists, overwriting (id: {}, type: {})", iconId, iconType);
            }

            m_fields->iconCache[iconType][iconId] = tex;
        }
    }
}

CCTexture2D* HookedGameManager::getCachedIcon(int iconId, int iconType) {
    if (m_fields->iconCache.contains(iconType)) {
        if (m_fields->iconCache.at(iconType).contains(iconId)) {
            auto* tex = *m_fields->iconCache.at(iconType).at(iconId);
            return tex;
        }
    }

    return nullptr;
}

bool HookedGameManager::getAssetsPreloaded() {
    return m_fields->assetsPreloaded;
}

void HookedGameManager::setAssetsPreloaded(bool state) {
    m_fields->assetsPreloaded = state;
}

void HookedGameManager::verifyCachedData() {
    if (m_fields->cachedSearchPathIdx == -1) {
        m_fields->cachedSearchPathIdx = util::cocos::findSearchPathIdxForFile("icons/robot_41-hd.png");
    }

    if (m_fields->cachedTexNameSuffix.empty()) {
        m_fields->cachedTexNameSuffix = util::cocos::getTextureNameSuffix();
    }
}