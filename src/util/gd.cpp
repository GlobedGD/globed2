#include "gd.hpp"

#include <defs/geode.hpp>
#include <data/types/gd.hpp>
#include <util/singleton.hpp>

using namespace geode::prelude;

namespace util::gd {
    void reorderDownloadedLevel(GJGameLevel* level) {
        // thank you cvolton :D
        // this is needed so the level appears at the top of the saved list (unless Manual Level Order is enabled)

        auto* levels = GameLevelManager::get()->m_onlineLevels;

        bool putAtLowest = globed::cachedSingleton<GameManager>()->getGameVariable("0084");

        int idx = 0;
        for (const auto& [k, level] : CCDictionaryExt<::gd::string, GJGameLevel*>(levels)) {
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

    void openProfile(int accountId, int userId, const std::string& name) {
        bool myself = accountId == GJAccountManager::get()->m_accountID;
        if (!myself) {
            GameLevelManager::sharedState()->storeUserName(userId, accountId, name);
        }

        ProfilePage::create(accountId, myself)->show();
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

    int getIconWithType(const PlayerIconData& data, PlayerIconType type) {
        int newIcon = data.cube;

        switch (type) {
            case PlayerIconType::Cube: newIcon = data.cube; break;
            case PlayerIconType::Ship: newIcon = data.ship; break;
            case PlayerIconType::Ball: newIcon = data.ball; break;
            case PlayerIconType::Ufo: newIcon = data.ufo; break;
            case PlayerIconType::Wave: newIcon = data.wave; break;
            case PlayerIconType::Robot: newIcon = data.robot; break;
            case PlayerIconType::Spider: newIcon = data.spider; break;
            case PlayerIconType::Swing: newIcon = data.swing; break;
            case PlayerIconType::Jetpack: newIcon = data.jetpack; break;
            default: newIcon = data.cube; break;
        };

        return newIcon;
    }

    int getIconWithType(const PlayerIconData& data, IconType type) {
        return getIconWithType(data, globed::into<PlayerIconType>(type));
    }

    static std::string gvkey(GameVariable var) {
        return fmt::format("{:04}", static_cast<int>(var));
    }

    bool variable(GameVariable var) {
        return globed::cachedSingleton<GameManager>()->getGameVariable(gvkey(var).c_str());
    }

    void setVariable(GameVariable var, bool state) {
        globed::cachedSingleton<GameManager>()->setGameVariable(gvkey(var).c_str(), state);
    }

    void safePopScene() {
        globed::cachedSingleton<GameManager>()->safePopScene();
        // auto dir = CCDirector::get();

        // if (dir->sceneCount() < 2) {
        //     auto ml = MenuLayer::scene(false);
        //     auto ct = CCTransitionFade::create(0.5f, ml);
        //     dir->replaceScene(ct);
        // } else {
        //     dir->popSceneWithTransition(0.5f, PopTransition::kPopTransitionFade);
        // }
    }
}
