#pragma once
#include <globed/core/data/FeaturedLevel.hpp>
#include <globed/prelude.hpp>

#include <Geode/Geode.hpp>
#include <Geode/modify/LevelCell.hpp>

namespace globed {

struct GLOBED_MODIFY_ATTR HookedLevelCell : public geode::Modify<HookedLevelCell, LevelCell> {
    struct Fields {
        CCLabelBMFont *m_playerCountLabel = nullptr;
        CCSprite *m_playerCountIcon = nullptr;
        std::optional<FeatureTier> m_rateTier;
    };

    void updatePlayerCount(int count, bool inLists = false);
    void setGlobedFeature(FeatureTier tier);
};

} // namespace globed