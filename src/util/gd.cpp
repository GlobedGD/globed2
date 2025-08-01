#include <globed/util/gd.hpp>
#include <globed/util/singleton.hpp>

using namespace geode::prelude;

namespace globed {

void reorderDownloadedLevel(GJGameLevel* level) {
    // thank you cvolton :D
    // this is needed so the level appears at the top of the saved list (unless Manual Level Order is enabled)

    auto* levels = GameLevelManager::get()->m_onlineLevels;

    bool putAtLowest = cachedSingleton<GameManager>()->getGameVariable("0084");

    int idx = 0;
    for (const auto& [k, level] : CCDictionaryExt<gd::string, GJGameLevel*>(levels)) {
        if (putAtLowest) {
            idx = std::min(idx, level->m_levelIndex);
        } else {
            idx = std::max(idx, level->m_levelIndex);
        }
    }

    if (putAtLowest) {
        idx -= 1;
    } else {
        idx += 1;
    }

    level->m_levelIndex = idx;
}

void pushScene(cocos2d::CCLayer* layer) {
    auto scene = CCScene::create();
    scene->addChild(layer);
    return pushScene(scene);
}

void pushScene(cocos2d::CCScene* scene) {
    // note: (applies to functions below too), fade transition is already removed by gd if fast menu is enabled
    // so we don't need to worry about that here
    cachedSingleton<CCDirector>()->pushScene(CCTransitionFade::create(.5f, scene));
}

void replaceScene(cocos2d::CCLayer* layer) {
    auto scene = CCScene::create();
    scene->addChild(layer);
    return replaceScene(scene);
}

void replaceScene(cocos2d::CCScene* scene) {
    cachedSingleton<CCDirector>()->replaceScene(CCTransitionFade::create(.5f, scene));
}

void popScene() {
    cachedSingleton<CCDirector>()->popSceneWithTransition(0.5f, PopTransition::kPopTransitionFade);
}

GameLevelKind classifyLevel(int levelId) {
    auto glm = GameLevelManager::get();

    // sanity check time!
    // m_mainLevels contains (blank) levels from gd world and spinoffs. yikes!
    // we should not return these to the user, instead returning a Custom kind.


    // main levels, tower levels, the challenge
    bool isRealMain = (levelId >= 1 && levelId <= 127) || (levelId >= 5001 && levelId <= 5024) || (levelId == 3001);

    auto mlevel = static_cast<GJGameLevel*>(glm->m_mainLevels->objectForKey(fmt::to_string(levelId)));

    if (!mlevel || !isRealMain) {
        return GameLevelKind {
            .level = nullptr,
            .kind = GameLevelKind::Custom
        };
    }

    if (levelId >= 1 && levelId <= 22) {
        // classic main levels
        return GameLevelKind {
            .level = mlevel,
            .kind = GameLevelKind::Main
        };
    } else {
        // tower levels or the challenge
        return GameLevelKind {
            .level = mlevel,
            .kind = GameLevelKind::Tower
        };
    }
}

}