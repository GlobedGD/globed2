#pragma once
#include <globed/config.hpp>

#include <Geode/Geode.hpp>
#include <Geode/modify/LevelCell.hpp>

namespace globed {

struct GLOBED_MODIFY_ATTR HookedLevelCell : geode::Modify<HookedLevelCell, LevelCell> {
    struct Fields {
        cocos2d::CCLabelBMFont* m_playerCountLabel = nullptr;
        cocos2d::CCSprite* m_playerCountIcon = nullptr;
    };

    void updatePlayerCount(int count, bool inLists = false);
};

}