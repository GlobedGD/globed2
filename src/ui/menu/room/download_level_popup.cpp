#include "download_level_popup.hpp"

#include <util/ui.hpp>

using namespace geode::prelude;

bool DownloadLevelPopup::setup(int levelId) {
    this->setTitle("Downloading level");

    auto winSize = CCDirector::get()->getWinSize();

    Build<CCLabelBMFont>::create("Fetching data..", "bigFont.fnt")
        .pos(winSize.width / 2, winSize.height / 2 + m_size.height / 2 - 50.f)
        .scale(0.35f)
        .parent(m_mainLayer)
        .store(statusLabel);

    auto* glm = GameLevelManager::sharedState();
    glm->m_levelManagerDelegate = this;
    glm->getOnlineLevels(GJSearchObject::create(SearchType::Search, std::to_string(levelId)));

    return true;
}

void DownloadLevelPopup::loadLevelsFinished(cocos2d::CCArray* levels, char const* p1, int p2) {
    if (levels->count() == 0) {
        this->onClose(this);
        FLAlertLayer::create("Error", "Failed to download the level. It may be unlisted or not exist on the servers anymore.", "Ok")->show();
        return;
    }

    auto level = static_cast<GJGameLevel*>(levels->objectAtIndex(0));

    auto* glm = GameLevelManager::sharedState();

    glm->m_levelManagerDelegate = nullptr;
    glm->m_levelDownloadDelegate = this;
    glm->downloadLevel(level->m_levelID, false);
}

void DownloadLevelPopup::loadLevelsFinished(cocos2d::CCArray* p0, char const* p1) {
    this->loadLevelsFinished(p0, p1, -1);
}

void DownloadLevelPopup::loadLevelsFailed(char const* p0, int p1) {
    this->onClose(this);
    FLAlertLayer::create("Error", fmt::format("Failed to download the level: {}", p1), "Ok")->show();
}

void DownloadLevelPopup::loadLevelsFailed(char const* p0) {
    this->loadLevelsFailed(p0, -1);
}

void DownloadLevelPopup::setupPageInfo(gd::string p0, char const* p1) {}

void DownloadLevelPopup::levelDownloadFinished(GJGameLevel* level) {
    this->onClose(this);
    util::ui::switchToScene(LevelInfoLayer::create(level, false));
}

void DownloadLevelPopup::levelDownloadFailed(int p0) {
    this->onClose(this);
    FLAlertLayer::create("Error", fmt::format("Failed to download the level: {}", p0), "Ok")->show();
}

void DownloadLevelPopup::onClose(cocos2d::CCObject* obj) {
    Popup::onClose(obj);
    auto* glm = GameLevelManager::sharedState();
    glm->m_levelDownloadDelegate = nullptr;
    glm->m_levelManagerDelegate = nullptr;
}

DownloadLevelPopup* DownloadLevelPopup::create(int levelId) {
    auto ret = new DownloadLevelPopup;
    if (ret->init(POPUP_WIDTH, POPUP_HEIGHT, levelId)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}