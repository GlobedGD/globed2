#include "level_list_layer.hpp"

#include <hooks/level_cell.hpp>
#include <data/packets/client/general.hpp>
#include <data/packets/server/general.hpp>
#include <net/manager.hpp>
#include <hooks/gjgamelevel.hpp>
#include <managers/error_queues.hpp>
#include <util/ui.hpp>
#include <Geode/loader/Dispatch.hpp>

using namespace geode::prelude;

GlobedSettings& settings = GlobedSettings::get();

bool GlobedLevelListLayer::init() {
    if (!CCLayer::init()) return false;

    auto winSize = CCDirector::get()->getWinSize();

    auto listview = Build<ListView>::create(CCArray::create(), 0.f, LIST_WIDTH, LIST_HEIGHT)
        .collect();

    Build<GJListLayer>::create(listview, "Levels", util::ui::BG_COLOR_BROWN, LIST_WIDTH, LIST_HEIGHT, 0)
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
        .pos(winSize.width - 35.f, 35.f)
        .intoNewParent(CCMenu::create())
        .pos(0.f, 0.f)
        .parent(this);

    constexpr float pageBtnPadding = 20.f;

    // pages buttons
    Build<CCSprite>::createSpriteName("GJ_arrow_03_001.png")
        .intoMenuItem([this](auto) {
            this->currentPage--;
            this->reloadPage();
        })
        .pos(pageBtnPadding, winSize.height / 2)
        .store(btnPagePrev)
        .intoNewParent(CCMenu::create())
        .pos(0.f, 0.f)
        .parent(this);

    CCSprite* btnSprite;
    Build<CCSprite>::createSpriteName("GJ_arrow_03_001.png")
        .store(btnSprite)
        .intoMenuItem([this](auto) {
            this->currentPage++;
            this->reloadPage();
        })
        .pos(winSize.width - pageBtnPadding, winSize.height / 2)
        .store(btnPageNext)
        .intoNewParent(CCMenu::create())
        .pos(0.f, 0.f)
        .parent(this);

    btnSprite->setFlipX(true);

    listLayer->setPosition(winSize / 2 - listLayer->getScaledContentSize() / 2);

    util::ui::prepareLayer(this);

    NetworkManager::get().addListener<LevelListPacket>(this, [this](std::shared_ptr<LevelListPacket> packet) {
        this->levelList.clear();
        this->levelPages.clear();
        this->sortedLevelIds.clear();

        for (const auto& level : packet->levels) {
            this->levelList.emplace(level.levelId, level.playerCount);
            this->sortedLevelIds.push_back(level.levelId);
        }

        // sort the levels
        auto comparator = [this](int a, int b) {
            if (!this->levelList.contains(a) || !this->levelList.contains(b)) return false;

            auto aVal = this->levelList.at(a);
            auto bVal = this->levelList.at(b);

            return aVal > bVal;
        };

        std::sort(sortedLevelIds.begin(), sortedLevelIds.end(), comparator);

        this->currentPage = 0;
        this->reloadPage();
    });

    this->refreshLevels();

    return true;
}

GlobedLevelListLayer::~GlobedLevelListLayer() {
    GameLevelManager::sharedState()->m_levelManagerDelegate = nullptr;
}

void GlobedLevelListLayer::reloadPage() {
    loading = true;

    this->showLoadingUi();

    btnPagePrev->setVisible(false);
    btnPageNext->setVisible(false);

    // if empty, don't make a request
    if (levelList.empty()) {
        this->loadLevelsFinished(CCArray::create(), "");
        return;
    }

    // if cached, reuse it
    if (currentPage < levelPages.size()) {
        auto& page = levelPages.at(currentPage);
        auto array = CCArray::create();
        for (const auto elem : page) {
            array->addObject(elem);
        }

        this->loadLevelsFinished(array, "");
        return;
    }

    size_t pageSize = settings.globed.increaseLevelList ? INCREASED_LIST_PAGE_SIZE : LIST_PAGE_SIZE;

    size_t startIdx = currentPage * pageSize;
    size_t endIdx = std::min((currentPage + 1) * pageSize, sortedLevelIds.size());

    // now join them to a comma separated string
    std::ostringstream oss;

    bool first = true;
    for (size_t i = startIdx; i < endIdx; i++) {
        if (first) {
            first = false;
        } else {
            oss << ",";
        }

        oss << sortedLevelIds[i];
    }

    auto glm = GameLevelManager::sharedState();
    glm->m_levelManagerDelegate = this;
    glm->getOnlineLevels(GJSearchObject::create((SearchType)26, oss.str()));
}

void GlobedLevelListLayer::loadListCommon() {
    loading = false;
    this->removeLoadingCircle();
    GameLevelManager::sharedState()->m_levelManagerDelegate = nullptr;
}

void GlobedLevelListLayer::removeLoadingCircle() {
    if (loadingCircle) {
        loadingCircle->fadeAndRemove();
        loadingCircle = nullptr;
    }
}

void GlobedLevelListLayer::showLoadingUi() {
    if (!loadingCircle) {
        Build<LoadingCircle>::create()
            .pos(0.f, 0.f)
            .store(loadingCircle);

        loadingCircle->setParentLayer(this);
        loadingCircle->show();
    }

    if (listLayer->m_listView) listLayer->m_listView->removeFromParent();
    listLayer->m_listView = Build<ListView>::create(CCArray::create(), 0.f, LIST_WIDTH, LIST_HEIGHT)
        .parent(listLayer)
        .collect();
}

void GlobedLevelListLayer::loadLevelsFinished(cocos2d::CCArray* p0, char const* p1, int p2) {
    this->loadListCommon();

    // guys im not gonna try and sort a ccarray manually

    std::vector<Ref<GJGameLevel>> sortedLevels;
    sortedLevels.reserve(p0->count());

    for (GJGameLevel* level : CCArrayExt<GJGameLevel*>(p0)) {
        sortedLevels.push_back(level);
    }

    // compare by player count (descending)
    auto comparator = [this](GJGameLevel* a, GJGameLevel* b) {
        LevelId levelIdA = HookedGJGameLevel::getLevelIDFrom(a);
        LevelId levelIdB = HookedGJGameLevel::getLevelIDFrom(b);
        if (!this->levelList.contains(levelIdA) || !this->levelList.contains(levelIdB)) return false;

        auto aVal = this->levelList.at(levelIdA);
        auto bVal = this->levelList.at(levelIdB);

        return aVal > bVal;
    };

    std::sort(sortedLevels.begin(), sortedLevels.end(), comparator);

    // add to cache
    if (levelPages.size() <= currentPage) {
        levelPages.push_back(sortedLevels);
    }

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
        int levelId = HookedGJGameLevel::getLevelIDFrom(cell->m_level);
        if (!levelList.contains(levelId)) continue;

        static_cast<GlobedLevelCell*>(cell)->updatePlayerCount(levelList.at(levelId));
    }

    // show the buttons
    if (currentPage > 0) {
        btnPagePrev->setVisible(true);
    }

    size_t pageSize = settings.globed.increaseLevelList ? INCREASED_LIST_PAGE_SIZE : LIST_PAGE_SIZE;

    if (currentPage < (sortedLevelIds.size() / pageSize)) {
        btnPageNext->setVisible(true);
    }
}

void GlobedLevelListLayer::loadLevelsFailed(char const* p0, int p1) {
    this->loadListCommon();
    log::warn("failed to load levels: {}, {}", p1, p0);
}

void GlobedLevelListLayer::loadLevelsFinished(cocos2d::CCArray* p0, char const* p1) {
    this->loadLevelsFinished(p0, p1, -1);
}

void GlobedLevelListLayer::loadLevelsFailed(char const* p0) {
    this->loadLevelsFailed(p0, -1);
}

void GlobedLevelListLayer::setupPageInfo(gd::string p0, const char* p1) {}

void GlobedLevelListLayer::refreshLevels() {
    if (loading) return;

    loading = true;
    btnPagePrev->setVisible(false);
    btnPageNext->setVisible(false);

    auto& nm = NetworkManager::get();
    if (!nm.established()) return;

    // request the level list from the server
    nm.send(RequestLevelListPacket::create());

    // remove existing listview and put a loading circle
    this->showLoadingUi();
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