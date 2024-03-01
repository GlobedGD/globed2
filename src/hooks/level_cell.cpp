#include "level_cell.hpp"
#include <defs/geode.hpp>

#include <util/math.hpp>

using namespace geode::prelude;

void GlobedLevelCell::updatePlayerCount(int count, bool inLists) {
    if (!m_fields->playerCountLabel) {
        float xPos = 0.f;
        float yPos = 0.f;

        if (util::math::equal(m_height, 90.f, 10.f)) {
            xPos = m_width - 12.f;
            yPos = 67.f;
        } else {
            // compact lists by cvolton
            xPos = m_width - 9.f;
            yPos = 6.5f;
        }

        Build<CCLabelBMFont>::create("", "goldFont.fnt")
            .scale(0.4f)
            .anchorPoint(1.f, 0.5f)
            .pos(xPos, yPos)
            .parent(this)
            .id("player-count-label"_spr)
            .store(m_fields->playerCountLabel);
    }

    if (count == 0) {
        m_fields->playerCountLabel->setVisible(false);
    } else {
        m_fields->playerCountLabel->setVisible(true);
        m_fields->playerCountLabel->setString(fmt::format("{} {}", count, count == 1 ? "player" : "players").c_str());
    }
}
