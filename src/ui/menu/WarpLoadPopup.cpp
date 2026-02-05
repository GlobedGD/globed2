#include "WarpLoadPopup.hpp"
#include <globed/core/PopupManager.hpp>
#include <globed/util/gd.hpp>

#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace globed {

WarpLoadPopup::~WarpLoadPopup() {
    auto* glm = GameLevelManager::sharedState();
    glm->m_levelDownloadDelegate = nullptr;
    glm->m_levelManagerDelegate = nullptr;
}

bool WarpLoadPopup::init(int levelId, bool openLevel, bool replaceScene) {
    if (!BasePopup::init(180.f, 80.f)) return false;

    this->setTitle("Loading Level");
    m_levelId = levelId;
    m_openLevel = openLevel;
    m_replaceScene = replaceScene;

    m_statusLabel = Build<CCLabelBMFont>::create("Fetching level data..", "bigFont.fnt")
        .pos(this->fromTop(50.f))
        .scale(0.35f)
        .parent(m_mainLayer);

    globed::getOnlineLevel(levelId, [this](auto level) {
        if (!level) {
            this->onClose(nullptr);
            globed::alertFormat("Error", "Failed to join the level. It is likely unlisted or deleted.");
            return;
        }

        this->onLoadedMeta(level);
    });

    // it's possible that a cached level was used and everything loaded instantly,
    // in this case don't even open the popup
    return !m_finished;
}

void WarpLoadPopup::onClose(cocos2d::CCObject* obj) {
    BasePopup::onClose(obj);
}

void WarpLoadPopup::onLoadedMeta(GJGameLevel* level) {
    if (!level->m_levelString.empty()) {
        // assume the level is already downloaded
        this->onLoadedData(level);
        return;
    }

    // download the level
    auto glm = GameLevelManager::get();

    glm->m_levelDownloadDelegate = this;
    glm->downloadLevel(level->m_levelID, false, 0);

    m_statusLabel->setString("Downloading level data..");
}

// Stage 2, level download

void WarpLoadPopup::levelDownloadFinished(GJGameLevel* level) {
    globed::reorderDownloadedLevel(level);
    this->onLoadedData(level);
}

void WarpLoadPopup::levelDownloadFailed(int p0) {
    this->onClose(this);
    globed::alertFormat("Error", "Failed to download the level (error code {})", p0);
    m_finished = true;
}

void WarpLoadPopup::onLoadedData(GJGameLevel* level) {
    bool open = m_openLevel;
    bool replace = m_replaceScene;
    bool shown = this->getParent() != nullptr;

    this->onClose(this);

    if (!shown) {
        m_finished = true;
    }

    // depending on whether open level is true or not, we should either open `LevelInfoLayer` or create a `PlayLayer`
    if (open) {
        auto scene = PlayLayer::scene(level, false, false);
        replace ? globed::replaceScene(scene) : globed::pushScene(scene);
    } else {
        auto layer = LevelInfoLayer::create(level, false);
        replace ? globed::replaceScene(layer) : globed::pushScene(layer);
    }
}

WarpLoadPopup* WarpLoadPopup::create(int levelId, bool openLevel, bool replaceScene) {
    auto ret = new WarpLoadPopup;
    if (ret->init(levelId, openLevel, replaceScene)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

}