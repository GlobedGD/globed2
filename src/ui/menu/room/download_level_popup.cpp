#include "download_level_popup.hpp"

#include <util/ui.hpp>
#include <util/gd.hpp>
#include <managers/popup.hpp>

using namespace geode::prelude;

bool DownloadLevelPopup::setup(int levelId) {
    this->setTitle("Downloading level");

    auto rlayout = util::ui::getPopupLayoutAnchored(m_size);

    Build<CCLabelBMFont>::create("Fetching data..", "bigFont.fnt")
        .pos(rlayout.fromTop(50.f))
        .scale(0.35f)
        .parent(m_mainLayer)
        .store(statusLabel);

    auto* glm = GameLevelManager::sharedState();
    glm->m_levelManagerDelegate = this;
    glm->getOnlineLevels(GJSearchObject::create(SearchType::Type26, std::to_string(levelId)));

    return true;
}

void DownloadLevelPopup::loadLevelsFinished(cocos2d::CCArray* levels, char const* p1, int p2) {
    if (!levels || levels->count() == 0) {
        this->onClose(this);
        PopupManager::get().alert("Error", "Failed to download the level. It may be unlisted or not exist on the servers anymore.").showInstant();
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
    PopupManager::get().alertFormat("Error", "Failed to download the level: {}", p1).showInstant();
}

void DownloadLevelPopup::loadLevelsFailed(char const* p0) {
    this->loadLevelsFailed(p0, -1);
}

void DownloadLevelPopup::setupPageInfo(gd::string p0, char const* p1) {}

void DownloadLevelPopup::levelDownloadFinished(GJGameLevel* level) {
    this->onClose(this);

    util::gd::reorderDownloadedLevel(level);
    util::ui::switchToScene(LevelInfoLayer::create(level, false));
}

void DownloadLevelPopup::levelDownloadFailed(int p0) {
    this->onClose(this);
    PopupManager::get().alert("Error", "Failed to download the level. It may be unlisted or not exist on the servers anymore.").showInstant();
}

void DownloadLevelPopup::onClose(cocos2d::CCObject* obj) {
    Popup::onClose(obj);
    auto* glm = GameLevelManager::sharedState();
    glm->m_levelDownloadDelegate = nullptr;
    glm->m_levelManagerDelegate = nullptr;
}

DownloadLevelPopup* DownloadLevelPopup::create(int levelId) {
    auto ret = new DownloadLevelPopup;
    if (ret->initAnchored(POPUP_WIDTH, POPUP_HEIGHT, levelId)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}