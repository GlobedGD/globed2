#include "level_list_layer.hpp"

#include <hooks/level_cell.hpp>
#include <data/packets/client/general.hpp>
#include <data/packets/server/general.hpp>
#include <net/network_manager.hpp>
#include <managers/error_queues.hpp>
#include <util/ui.hpp>

using namespace geode::prelude;

bool GlobedLevelListLayer::init() {
    if (!CCLayer::init()) return false;

    auto winSize = CCDirector::get()->getWinSize();

    auto listview = Build<ListView>::create(CCArray::create(), 0.f, LIST_WIDTH, LIST_HEIGHT)
        .collect();

    Build<GJListLayer>::create(listview, "Levels", ccc4(0, 0, 0, 180), LIST_WIDTH, LIST_HEIGHT, 0)
        .zOrder(2)
        .anchorPoint(0.f, 0.f)
        .parent(this)
        .id("level-list"_spr)
        .store(listLayer);

    // refresh button
    Build<CCSprite>::createSpriteName("GJ_updateBtn_001.png")
        .intoMenuItem([this](auto) {
            this->refreshLevels();
        })
        .pos(winSize.width - 40.f, 40.f)
        .intoNewParent(CCMenu::create())
        .pos(0.f, 0.f)
        .parent(this);

    listLayer->setPosition(winSize / 2 - listLayer->getScaledContentSize() / 2);

    util::ui::prepareLayer(this);

    NetworkManager::get().addListener<LevelListPacket>([this](LevelListPacket* packet) {
        this->levelList.clear();
        for (const auto& level : packet->levels) {
            this->levelList.emplace(level.levelId, level.playerCount);
        }
        this->onLevelsReceived();
    });

    this->refreshLevels();

    return true;
}

GlobedLevelListLayer::~GlobedLevelListLayer() {
    NetworkManager::get().removeListener<LevelListPacket>();
    GameLevelManager::sharedState()->m_levelManagerDelegate = nullptr;
}

void GlobedLevelListLayer::onLevelsReceived() {
    loadingState = LoadingState::WaitingRobtop;

    log::debug("downloading {} levels", levelList.size());

    std::ostringstream oss;

    bool first = true;
    for (const auto& [levelId, _] : levelList) {
        if (first) {
            first = false;
        } else {
            oss << ",";
        }

        oss << levelId;
    }

    auto glm = GameLevelManager::sharedState();
    glm->m_levelManagerDelegate = this;
    glm->getOnlineLevels(GJSearchObject::create((SearchType)26, oss.str()));
}

void GlobedLevelListLayer::loadListCommon() {
    loadingState = LoadingState::Idle;
    loadingCircle->fadeAndRemove();
    loadingCircle = nullptr;
    GameLevelManager::sharedState()->m_levelManagerDelegate = nullptr;
}

void GlobedLevelListLayer::loadLevelsFinished(cocos2d::CCArray* p0, char const* p1, int p2) {
    this->loadListCommon();

    // guys im not gonna try and sort a ccarray manually

    std::vector<GJGameLevel*> sortedLevels;
    sortedLevels.reserve(p0->count());

    for (GJGameLevel* level : CCArrayExt<GJGameLevel*>(p0)) {
        level->m_gauntletLevel = false;
        level->m_gauntletLevel2 = false;
        // TODO globed 2.1 did some caching of gjgamelevels here, not sure if it's really needed

        sortedLevels.push_back(level);
    }

    // compare by player count (descending)
    auto comparator = [this](GJGameLevel* a, GJGameLevel* b) {
        auto aVal = this->levelList.at(a->m_levelID);
        auto bVal = this->levelList.at(b->m_levelID);

        return aVal > bVal;
    };

    std::sort(sortedLevels.begin(), sortedLevels.end(), comparator);

    CCArray* finalArray = CCArray::create();
    for (GJGameLevel* level : sortedLevels) {
        finalArray->addObject(level);
    }

    if (listLayer->m_listView) listLayer->m_listView->removeFromParent();
    listLayer->m_listView = Build<CustomListView>::create(finalArray, BoomListType::Level, LIST_HEIGHT, LIST_WIDTH)
        .parent(listLayer)
        .collect();

    // guys we are about to do a funny
    for (LevelCell* cell : CCArrayExt<LevelCell*>(listLayer->m_listView->m_tableView->m_contentLayer->getChildren())) {
        static_cast<GlobedLevelCell*>(cell)->updatePlayerCount(levelList.at(cell->m_level->m_levelID.value()));
    }
}

void GlobedLevelListLayer::loadLevelsFailed(char const* p0, int p1) {
    this->loadListCommon();
    ErrorQueues::get().warn(p0);
}

void GlobedLevelListLayer::loadLevelsFinished(cocos2d::CCArray* p0, char const* p1) {
    this->loadLevelsFinished(p0, p1, -1);
}

void GlobedLevelListLayer::loadLevelsFailed(char const* p0) {
    this->loadLevelsFailed(p0, -1);
}

void GlobedLevelListLayer::setupPageInfo(gd::string p0, const char* p1) {}

void GlobedLevelListLayer::refreshLevels() {
    if (loadingState != LoadingState::Idle) return;

    loadingState = LoadingState::WaitingServer;

    // request the level list from the server
    auto& nm = NetworkManager::get();
    nm.send(RequestLevelListPacket::create());

    // remove existing listview and put a loading circle
    if (listLayer->m_listView) listLayer->m_listView->removeFromParent();
    listLayer->m_listView = Build<ListView>::create(CCArray::create(), 0.f, LIST_WIDTH, LIST_HEIGHT)
        .parent(listLayer)
        .collect();

    Build<LoadingCircle>::create()
        .pos(0.f, 0.f)
        .store(loadingCircle);

    loadingCircle->setParentLayer(this);
    loadingCircle->show();
}

void GlobedLevelListLayer::keyBackClicked() {
    util::ui::navigateBack();
}

GlobedLevelListLayer* GlobedLevelListLayer::create() {
    auto ret = new GlobedLevelListLayer;
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}