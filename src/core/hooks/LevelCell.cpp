#include "LevelCell.hpp"
#include <globed/core/SettingsManager.hpp>
#include <ui/menu/FeatureCommon.hpp>
#include <ui/misc/CellGradients.hpp>

#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace globed {

void HookedLevelCell::updatePlayerCount(int count, bool inLists) {
    auto& fields = *m_fields.self();

    if (!fields.m_playerCountIcon || !fields.m_playerCountLabel) {
        float xPos = 0.f;
        float yPos = 0.f;
        float iconScale = 0.f;

        if (std::abs(m_height - 90.f) < 10.f) {
            xPos = m_width - 12.f;
            yPos = 67.f;
            iconScale = 0.4f;
        } else {
            // compact lists by cvolton
            xPos = m_width - 9.f;
            yPos = 6.5f;
            iconScale = .3f;
        }

        fields.m_playerCountLabel = Build<CCLabelBMFont>::create("", "goldFont.fnt")
            .scale(0.4f)
            .anchorPoint(1.f, 0.5f)
            .pos(xPos, yPos)
            .parent(this)
            .id("player-count-label"_spr);

        fields.m_playerCountIcon = Build<CCSprite>::create("icon-person.png"_spr)
            .scale(iconScale)
            .anchorPoint(1.f, 0.5f)
            .pos(xPos, yPos)
            .parent(this)
            .id("player-count-icon"_spr);
    }

    if (!fields.m_playerCountLabel || !fields.m_playerCountLabel) {
        log::error("failed to create sprite in LevelCell");
        return;
    }

    if (count == 0) {
        fields.m_playerCountIcon->setVisible(false);
        fields.m_playerCountLabel->setVisible(false);
        return;
    }

    fields.m_playerCountLabel->setVisible(true);

    if (globed::setting<bool>("core.ui.compressed-player-count")) {
        fields.m_playerCountIcon->setVisible(true);
        fields.m_playerCountLabel->setString(fmt::format("{}", count).c_str());
        fields.m_playerCountLabel->setPositionX(fields.m_playerCountIcon->getPositionX() - 13.f);
    } else {
        fields.m_playerCountIcon->setVisible(false);
        fields.m_playerCountLabel->setString(fmt::format("{} {}", count, count == 1 ? "player" : "players").c_str());
    }
}

void HookedLevelCell::setGlobedFeature(FeatureTier tier) {
    auto& fields = *m_fields.self();
    fields.m_rateTier = tier;

    if (auto diff = globed::findDifficultySprite(this)) {
        globed::attachRatingSprite(diff, tier);
        globed::addCellGradient(this, globed::CellGradientType::FriendIngame);
    }
}

}
