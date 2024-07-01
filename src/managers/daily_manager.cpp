#include "daily_manager.hpp"
#include "Geode/binding/GameToolbox.hpp"
#include <util/ui.hpp>
#include <algorithm>

using namespace geode::prelude;

void DailyManager::setStoredLevel(GJGameLevel* level) {
    storedLevel = level;
}

GJGameLevel* DailyManager::getStoredLevel() {
    return storedLevel;
}

void DailyManager::requestDailyItems() {
    dailyLevelsList.clear();

    dailyLevelsList.push_back({102837084, 1, 2});
    dailyLevelsList.push_back({107224663, 2, 2});
    dailyLevelsList.push_back({105117679, 3, 0});
    dailyLevelsList.push_back({100643019, 4, 1});
}

int DailyManager::getRatingFromID(int levelId) {
    int rating = -1;
    for (DailyItem item : dailyLevelsList) {
        if (item.levelId == levelId) {
            rating = item.rateTier;
            break;
        }
    }
    return rating;
}

std::vector<long long> DailyManager::getLevelIDs() {
    std::vector<long long> vec;
    for (DailyItem item : dailyLevelsList) {
        //log::info("test {}", item.levelId);
        vec.push_back(item.levelId);
    }
    return vec;
}

std::vector<long long> DailyManager::getLevelIDsReverse() {
    std::vector<long long> vec;
    for (DailyItem item : dailyLevelsList) {
        //log::info("test {}", item.levelId);
        vec.push_back(item.levelId);
    }
    std::reverse(vec.begin(), vec.end());
    return vec;
}

DailyItem DailyManager::getRecentDailyItem() {
    return dailyLevelsList.back();
}

void DailyManager::attachRatingSprite(int tier, CCNode* parent) {
    if (parent->getChildByID("globed-rating"_spr)) {
        return;
    }

    for (CCNode* child : CCArrayExt<CCNode*>(parent->getChildren())) {
        child->setVisible(false);
    }

    CCSprite* spr;
    switch(tier) {
        case 1:
            spr = CCSprite::createWithSpriteFrameName("icon-epic.png"_spr);
            break;
        case 2:
            spr = CCSprite::createWithSpriteFrameName("icon-outstanding.png"_spr);
            break;
        case 0:
        default:
            spr = CCSprite::createWithSpriteFrameName("icon-featured.png"_spr);
            break;
    }

    spr->setZOrder(-1);
    spr->setPosition(parent->getScaledContentSize() / 2);
    spr->setID("globed-rating"_spr);
    parent->addChild(spr);

    if (tier == 2) {
        CCSprite* overlay = Build<CCSprite>::createSpriteName("icon-outstanding-overlay.png"_spr)
            .pos(parent->getScaledContentSize() / 2 + CCPoint{0.f, 2.f})
            .blendFunc({GL_ONE, GL_ONE})
            .color({200, 255, 255})
            .opacity(175)
            .zOrder(1)
            .parent(parent);

        auto particle = GameToolbox::particleFromString("26a-1a1.25a0.3a16a90a62a4a0a20a20a0a16a0a0a0a0a4a2a0a0a0.341176a0a1a0a0.635294a0a1a0a0a1a0a0a0.247059a0a1a0a0.498039a0a1a0a0.16a0a0.23a0a0a0a0a0a0a0a0a2a1a0a0a0a0a0a0a0a0a0a0a0a0a0a0a0a0a0a0", NULL, false);
        particle->setPosition({parent->getScaledContentWidth() / 2, parent->getScaledContentHeight() / 2 + 4.f});
        particle->setZOrder(-2);
        parent->addChild(particle);
    }
}

GJSearchObject* DailyManager::getSearchObject() {
    std::stringstream download;
    bool first = true;

    for (int i = dailyLevelsList.size() - 1; i >= 0; i--) {
        if (!first) {
            download << ",";
        }

        download << dailyLevelsList.at(i).levelId;
        first = false;
    }

    GJSearchObject* searchObj = GJSearchObject::create(SearchType::Type19, download.str());
    return searchObj;
}
