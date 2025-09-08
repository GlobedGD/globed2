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

static std::string gvkey(GameVariable var) {
    return fmt::format("{:04}", static_cast<int>(var));
}

bool gameVariable(GameVariable var) {
    return globed::cachedSingleton<GameManager>()->getGameVariable(gvkey(var).c_str());
}

void setGameVariable(GameVariable var, bool state) {
    globed::cachedSingleton<GameManager>()->setGameVariable(gvkey(var).c_str(), state);
}

const char* difficultyToString(Difficulty diff) {
    switch (diff) {
        case Difficulty::Auto: return "Auto";
        case Difficulty::NA: return "NA";
        case Difficulty::Easy: return "Easy";
        case Difficulty::Normal: return "Normal";
        case Difficulty::Hard: return "Hard";
        case Difficulty::Harder: return "Harder";
        case Difficulty::Insane: return "Insane";
        case Difficulty::HardDemon: return "HardDemon";
        case Difficulty::EasyDemon: return "EasyDemon";
        case Difficulty::MediumDemon: return "MediumDemon";
        case Difficulty::InsaneDemon: return "InsaneDemon";
        case Difficulty::ExtremeDemon: return "ExtremeDemon";
        default: return "NA";
    }
}

std::optional<Difficulty> difficultyFromString(std::string_view diff) {
    if (diff == "Auto") return Difficulty::Auto;
    if (diff == "NA") return Difficulty::NA;
    if (diff == "Easy") return Difficulty::Easy;
    if (diff == "Normal") return Difficulty::Normal;
    if (diff == "Hard") return Difficulty::Hard;
    if (diff == "Harder") return Difficulty::Harder;
    if (diff == "Insane") return Difficulty::Insane;
    if (diff == "HardDemon") return Difficulty::HardDemon;
    if (diff == "EasyDemon") return Difficulty::EasyDemon;
    if (diff == "MediumDemon") return Difficulty::MediumDemon;
    if (diff == "InsaneDemon") return Difficulty::InsaneDemon;
    if (diff == "ExtremeDemon") return Difficulty::ExtremeDemon;

    return std::nullopt;
}

Difficulty calcLevelDifficulty(GJGameLevel* level) {
    using enum Difficulty;

    Difficulty diff = NA;
    // "would a backwards wormhole be a whitehole or a holeworm?" - kiba 2024
    if (level->m_autoLevel) {
        diff = Auto;
    } else if (level->m_ratingsSum != 0) {
        if (level->m_demon == 1){
            int fixedNum = std::clamp(level->m_demonDifficulty, 0, 6);

            if (fixedNum != 0) {
                fixedNum -= 2;
            }

            diff = static_cast<Difficulty>(6 + fixedNum);
        } else {
            int val = level->m_ratingsSum / level->m_ratings;
            if (val > (int) Difficulty::ExtremeDemon) {
                val = (int) Difficulty::ExtremeDemon;
            } else if (val < (int) Difficulty::NA) {
                val = (int) Difficulty::NA;
            }

            diff = static_cast<Difficulty>(val);
        }
    }

    return diff;
}

cue::Icons getPlayerIcons() {
    auto gm = cachedSingleton<GameManager>();

    return cue::Icons {
        .type = IconType::Cube,
        .id = gm->m_playerFrame,
        .color1 = gm->m_playerColor,
        .color2 = gm->m_playerColor2,
        .glowColor = gm->m_playerGlow ? gm->m_playerGlowColor : -1,
    };
}

}