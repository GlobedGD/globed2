#include "WarpLoadPopup.hpp"
#include <globed/core/PopupManager.hpp>
#include <globed/util/gd.hpp>

#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace globed {

const cocos2d::CCSize WarpLoadPopup::POPUP_SIZE = { 180.f, 80.f };

WarpLoadPopup::~WarpLoadPopup() {
    auto* glm = GameLevelManager::sharedState();
    glm->m_levelDownloadDelegate = nullptr;
    glm->m_levelManagerDelegate = nullptr;
}

bool WarpLoadPopup::setup(int levelId, bool openLevel) {
    this->setTitle("Loading Level");
    m_levelId = levelId;
    m_openLevel = openLevel;

    m_statusLabel = Build<CCLabelBMFont>::create("Fetching level data..", "bigFont.fnt")
        .pos(this->fromTop(50.f))
        .scale(0.35f)
        .parent(m_mainLayer);

    auto glm = GameLevelManager::get();
    glm->m_levelManagerDelegate = this;
    glm->getOnlineLevels(GJSearchObject::create(SearchType::Search, fmt::to_string(levelId)));

    return true;
}

void WarpLoadPopup::onClose(cocos2d::CCObject* obj) {
    BasePopup::onClose(obj);
}

void WarpLoadPopup::loadLevelsFinished(cocos2d::CCArray* levels, char const* p1, int p2) {
    if (levels->count() == 0) {
        this->onClose(nullptr);
        globed::alertFormat("Error", "Failed to load data for the level (no results for ID {}). It could be unlisted or deleted.", m_levelId);
        return;
    }

    auto level = static_cast<GJGameLevel*>(levels->objectAtIndex(0));
    auto glm = GameLevelManager::get();

    glm->m_levelManagerDelegate = nullptr;
    glm->m_levelDownloadDelegate = this;
    glm->downloadLevel(level->m_levelID, false);

    m_statusLabel->setString("Downloading level data..");
}

void WarpLoadPopup::loadLevelsFinished(cocos2d::CCArray* p0, char const* p1) {
    this->loadLevelsFinished(p0, p1, -1);
}

void WarpLoadPopup::loadLevelsFailed(char const* p0, int p1) {
    this->onClose(nullptr);
    globed::alertFormat("Error", "Failed to load data for the level {} (error code {}). It could be unlisted or deleted.", m_levelId, p1);
}

void WarpLoadPopup::loadLevelsFailed(char const* p0) {
    this->loadLevelsFailed(p0, -1);
}

// Stage 2, level download

void WarpLoadPopup::levelDownloadFinished(GJGameLevel* level) {
    this->onClose(this);

    globed::reorderDownloadedLevel(level);

    // depending on whether open level is true or not, we should either open `LevelInfoLayer` or create a `PlayLayer`

    if (m_openLevel) {
        globed::pushScene(PlayLayer::scene(level, false, false));
    } else {
        globed::pushScene(LevelInfoLayer::create(level, false));
    }
}

void WarpLoadPopup::levelDownloadFailed(int p0) {
    this->onClose(this);
    globed::alertFormat("Error", "Failed to download the level (error code {})", p0);
}

}