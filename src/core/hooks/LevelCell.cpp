#include "LevelCell.hpp"
#include <globed/core/SettingsManager.hpp>

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

        fields.m_playerCountIcon = Build<CCSprite>::createSpriteName("icon-person.png"_spr)
            .scale(iconScale)
            .anchorPoint(1.f, 0.5f)
            .pos(xPos, yPos)
            .parent(this)
            .id("player-count-icon"_spr);
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
    } else {
        fields.m_playerCountIcon->setVisible(false);
        fields.m_playerCountLabel->setString(fmt::format("{} {}", count, count == 1 ? "player" : "players").c_str());
    }
}

}
