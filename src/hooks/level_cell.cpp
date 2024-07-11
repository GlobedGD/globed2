#include "level_cell.hpp"
#include "ui/ui.hpp"

#include <defs/geode.hpp>
#include <managers/settings.hpp>
#include <managers/daily_manager.hpp>
#include <util/math.hpp>


using namespace geode::prelude;

void GlobedLevelCell::onClick(CCObject* sender)  {
    DailyManager::get().rateTierOpen = this->m_fields->rateTier;
    LevelCell::onClick(sender);
    DailyManager::get().rateTierOpen = -1;
}

void GlobedLevelCell::modifyToFeaturedCell(int rating) {
    if (rating != -1) {
        auto* diff = DailyManager::get().findDifficultySprite(this);

        if (diff) {
            DailyManager::get().attachRatingSprite(rating, diff);

            float borderWidth = 1.f;
            float offset = borderWidth;

            CCPoint rect[4] = {
                CCPoint(0 + offset, 0 + offset),
                CCPoint(0 + offset, this->getScaledContentHeight() - offset),
                CCPoint(this->getScaledContentWidth() - offset, this->getScaledContentHeight() - offset),
                CCPoint(this->getScaledContentWidth() - offset, 0 + offset)
            };

            CCSprite* gradient = Build<CCSprite>::createSpriteName("friend-gradient.png"_spr)
            .color(globed::into<ccColor3B>(globed::color::FriendIngameGradient))
            .anchorPoint({0, 0})
            .opacity(90)
            .zOrder(-5)
            .parent(this);

            gradient->setScaleX(this->getScaledContentWidth() / gradient->getScaledContentWidth() / 2);
            gradient->setScaleY(this->getScaledContentHeight() / gradient->getScaledContentHeight());
        }
    }
}

void GlobedLevelCell::updatePlayerCount(int count, bool inLists) {
    auto& settings = GlobedSettings::get();

    if (!m_fields->playerCountLabel || !m_fields->playerCountIcon) {
        float xPos = 0.f;
        float yPos = 0.f;
        float iconScale = 0.f;

        if (util::math::equal(m_height, 90.f, 10.f)) {
            xPos = m_width - 12.f;
            yPos = 67.f;
            iconScale = .4f;
        } else {
            // compact lists by cvolton
            xPos = m_width - 9.f;
            yPos = 6.5f;
            iconScale = .3f;
        }

        Build<CCLabelBMFont>::create("", "goldFont.fnt")
            .scale(0.4f)
            .anchorPoint(1.f, 0.5f)
            .pos(xPos, yPos)
            .parent(this)
            .id("player-count-label"_spr)
            .store(m_fields->playerCountLabel);

        Build<CCSprite>::createSpriteName("icon-person.png"_spr)
                .scale(iconScale)
                .anchorPoint(1.f, 0.5f)
                .pos(xPos, yPos)
                .parent(this)
                .id("player-count-icon"_spr)
                .store(m_fields->playerCountIcon);

        m_fields->playerCountIcon->setVisible(false); // :D

        if (settings.globed.compressedPlayerCount) {
            m_fields->playerCountLabel->setPositionX(xPos - 13);
        }
    }

    if (count == 0) {
        m_fields->playerCountLabel->setVisible(false);
        m_fields->playerCountIcon->setVisible(false);
    } else {
        m_fields->playerCountLabel->setVisible(true);

        if (settings.globed.compressedPlayerCount) {
            m_fields->playerCountIcon->setVisible(true);
            m_fields->playerCountLabel->setString(fmt::format("{}", count).c_str());
        } else {
            m_fields->playerCountLabel->setString(fmt::format("{} {}", count, count == 1 ? "player" : "players").c_str());
        }

    }
}
