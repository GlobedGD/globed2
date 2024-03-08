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

    for (const auto& range : ranges) {
        for (int id = range.startId; id <= range.endId; id++) {
            auto sheetName = this->sheetNameForIcon(id, range.iconType);
            if (sheetName.empty()) continue;

            toLoad.push_back(sheetName);
        }
    }

    log::debug("preparing to load {} batched icons", toLoad.size());

    util::cocos::loadAssetsParallel(toLoad);

    log::debug("loaded successfully.");

    auto* tc = CCTextureCache::sharedTextureCache();

    auto texNameSuffix = util::cocos::getTextureNameSuffix();
    auto plistNameSuffix = util::cocos::getTextureNameSuffixPlist();
    size_t searchPathIdx = util::cocos::findSearchPathIdxForFile("icons/robot_41-hd.png");

    for (const auto& range : ranges) {
        for (int id = range.startId; id <= range.endId; id++) {
            const auto& sheetName = toLoad[id - range.startId];

            auto fullpath = util::cocos::fullPathForFilename(fmt::format("{}.png", sheetName), searchPathIdx);
            if (fullpath.empty()) {
                log::warn("path empty: {}", sheetName);
                continue;
            }

            std::string transformed = util::cocos::transformPathWithQuality(fullpath, texNameSuffix);

            auto* tex = static_cast<CCTexture2D*>(tc->m_pTextures->objectForKey(transformed));
            // log::debug("tex for {}: {}", transformed, tex);

            if (!tex) {
                log::warn("icon failed to preload: type {}, id {}", range.iconType, id);
                continue;
            }

            m_fields->iconCache[range.iconType][id] = tex;
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
