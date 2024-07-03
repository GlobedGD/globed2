#include "gd.hpp"

#include <defs/geode.hpp>

using namespace geode::prelude;

namespace util::gd {
    void reorderDownloadedLevel(GJGameLevel* level) {
        // thank you cvolton :D
        // this is needed so the level appears at the top of the saved list (unless Manual Level Order is enabled)

        auto* levels = GameLevelManager::get()->m_onlineLevels;

        bool putAtLowest = GameManager::get()->getGameVariable("0084");

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
}
