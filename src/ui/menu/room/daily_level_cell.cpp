#include "daily_level_cell.hpp"
#include <util/ui.hpp>
#include <net/manager.hpp>
#include <util/ui.hpp>
#include <hooks/level_select_layer.hpp>
#include <hooks/gjgamelevel.hpp>

using namespace geode::prelude;

bool GlobedDailyLevelCell::init(int levelId, int edition, int rateTier) {
    if (!CCLayer::init()) return false;

    auto winSize = CCDirector::sharedDirector()->getWinSize();

    darkBackground = Build<CCScale9Sprite>::create("square02_001.png")
    .contentSize({CELL_WIDTH, CELL_HEIGHT})
    .opacity(75)
    .zOrder(2)
    .parent(this);
    
    auto* glm = GameLevelManager::sharedState();
    glm->m_levelManagerDelegate = this;
    glm->getOnlineLevels(GJSearchObject::create(SearchType::Search, std::to_string(levelId)));

    loadingCircle = Build<LoadingCircle>::create()
    .zOrder(-5)
    .pos(winSize * -0.5)
    .opacity(100)
    .parent(this);
    // dont replace this with ->show() and look in the bottom left!!! (worst mistake of my life)
    loadingCircle->runAction(CCRepeatForever::create(CCSequence::create(CCRotateBy::create(1.f, 360.f), nullptr)));

    return true;
}

////////////////////
void GlobedDailyLevelCell::levelDownloadFinished(GJGameLevel* level) {
    loadingCircle->fadeAndRemove();
    
    Build<CCScale9Sprite>::create("GJ_square02.png")
    .contentSize({CELL_WIDTH, CELL_HEIGHT})
    .zOrder(5)
    .pos(darkBackground->getScaledContentSize() / 2)
    .parent(darkBackground)
    .store(background);

    Build<CCMenu>::create()
    .zOrder(6)
    .pos(background->getScaledContentSize() / 2)
    .parent(background)
    .store(menu);

    auto crown = Build<CCSprite>::createSpriteName("icon-crown.png"_spr)
    .pos({background->getScaledContentWidth() / 2, CELL_HEIGHT + 11.f})
    .zOrder(6)
    .parent(background);

    int levelId = level->m_levelID;

    auto levelbtntest =  Build<CCSprite>::createSpriteName("GJ_playBtn2_001.png")
        .intoMenuItem([this, level](auto) {
            util::ui::switchToScene(LevelInfoLayer::create(level, false));
        })
        .parent(menu)
        .zOrder(10)
        .id("play-button"_spr);
}

void GlobedDailyLevelCell::levelDownloadFailed(int) {
    loadingCircle->fadeAndRemove();
}
////////////////////

void GlobedDailyLevelCell::loadLevelsFinished(cocos2d::CCArray* levels, char const* p1, int p2) {
    if (levels->count() == 0) {
        return;
    }

    auto level = static_cast<GJGameLevel*>(levels->objectAtIndex(0));
    auto* glm = GameLevelManager::sharedState();
    glm->m_levelManagerDelegate = nullptr;
    glm->m_levelDownloadDelegate = this;
    glm->downloadLevel(level->m_levelID, false);
}

void GlobedDailyLevelCell::loadLevelsFinished(cocos2d::CCArray* p0, char const* p1) {
    this->loadLevelsFinished(p0, p1, -1);
}

void GlobedDailyLevelCell::loadLevelsFailed(char const* p0, int p1) {
    FLAlertLayer::create("Error", fmt::format("Failed to download the level: {}", p1), "Ok")->show();
}

void GlobedDailyLevelCell::loadLevelsFailed(char const* p0) {
    this->loadLevelsFailed(p0, -1);
}

GlobedDailyLevelCell* GlobedDailyLevelCell::create(int levelId, int edition, int rateTier) {
    auto ret = new GlobedDailyLevelCell;
    if (ret->init(levelId, edition, rateTier)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}