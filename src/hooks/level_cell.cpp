#include "level_cell.hpp"

using namespace geode::prelude;

void GlobedLevelCell::updatePlayerCount(int count, bool inLists) {
    if (!m_fields->playerCountLabel) {
        Build<CCLabelBMFont>::create("", "goldFont.fnt")
            .scale(0.4f)
            .anchorPoint(1.f, 0.5f)
            .pos(m_width - 8.f, inLists ? 40.f : 68.f)
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
