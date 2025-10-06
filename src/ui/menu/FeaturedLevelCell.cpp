#include "FeaturedLevelCell.hpp"
#include <globed/core/RoomManager.hpp>
#include <globed/util/gd.hpp>
#include <core/net/NetworkManagerImpl.hpp>
#include <ui/menu/FeatureCommon.hpp>

#include <UIBuilder.hpp>
#include <cue/Util.hpp>

using namespace geode::prelude;

namespace globed {

class NewLevelCell : public LevelCell {
public:
    void draw() override {
        // balls
        // balls
        // balls
    }

    FeatureTier rateTier = FeatureTier::Normal;

    NewLevelCell(char const* p0, float p1, float p2) : LevelCell(p0, p1, p2) {}
};

bool FeaturedLevelCell::init() {
    if (!CCLayer::init()) return false;

    this->ignoreAnchorPointForPosition(false);
    this->setContentSize({CELL_WIDTH, CELL_HEIGHT});

    auto winSize = CCDirector::get()->getWinSize();

    m_bg = Build<CCScale9Sprite>::create("square02_001.png")
        .anchorPoint(0.f, 0.f)
        .contentSize({CELL_WIDTH, CELL_HEIGHT})
        .opacity(75)
        .zOrder(10)
        .parent(this);

    m_loadedContainer = Build<CCNode>::create()
        .anchorPoint(0.f, 0.f)
        .contentSize(CELL_WIDTH, CELL_HEIGHT)
        .zOrder(10)
        .visible(false)
        .parent(this);

    m_loadedBg = Build<CCScale9Sprite>::create("GJ_square02.png")
        .anchorPoint(0.f, 0.f)
        .contentSize({CELL_WIDTH, CELL_HEIGHT})
        .zOrder(9)
        .visible(false)
        .parent(this);

    m_loadingCircle = Build<cue::LoadingCircle>::create(true)
        .zOrder(15)
        .opacity(100)
        .pos(CELL_WIDTH / 2.f, CELL_HEIGHT / 2.f)
        .parent(this);

    this->reload();

    auto& nm = NetworkManagerImpl::get();

    m_playerCountListener = nm.listen<msg::PlayerCountsMessage>([this](const auto& packet) {
        if (!packet.counts.empty()) {
            auto count = packet.counts[0].second;
            this->updatePlayerCount(count);
        }

        return ListenerResult::Continue;
    });

    m_levelListener = nm.listen<msg::FeaturedLevelMessage>([this](const auto& packet) {
        this->reload(true);
        return ListenerResult::Continue;
    });

    return true;
}

FeaturedLevelCell::~FeaturedLevelCell() {
    GameLevelManager::get()->m_levelManagerDelegate = nullptr;
}

void FeaturedLevelCell::createCell() {
    m_loadedContainer->setVisible(true);
    m_loadedBg->setVisible(true);
    m_bg->setVisible(false);

    NetworkManagerImpl::get().sendRequestPlayerCounts(RoomManager::get().makeSessionId(m_level->m_levelID));

    Build<CCMenu>::create()
        .zOrder(6)
        .pos({CELL_WIDTH - 75, CELL_HEIGHT / 2})
        .parent(m_loadedContainer)
        .collect();

    auto crown = Build<CCSprite>::create("icon-crown.png"_spr)
        .pos({m_bg->getScaledContentWidth() / 2, CELL_HEIGHT + 9.f})
        .scale(0.6f)
        .zOrder(6)
        .parent(m_loadedContainer)
        .collect();

    int frameValue = static_cast<int>(m_level->m_difficulty);

    auto levelcell = new NewLevelCell("baller", CELL_WIDTH - 15, CELL_HEIGHT - 25);
    levelcell->autorelease();
    levelcell->loadFromLevel(m_level);
    levelcell->setPosition({7.5f, 12.5f});
    levelcell->rateTier = m_levelMeta->rateTier;
    m_loadedContainer->addChild(levelcell);

    auto menu = levelcell->m_mainLayer->getChildByType<CCMenu>(0);
    if (!menu) return;

    auto toggler = menu->getChildByType<CCMenuItemToggler>(0);

    if (toggler) {
        toggler->setVisible(false);
    }

    auto cvoltonID = levelcell->m_mainLayer->getChildByIDRecursive("cvolton.betterinfo/level-id-label");
    if (cvoltonID != nullptr) {
        cvoltonID->setVisible(false);
    }

    auto playBtn = typeinfo_cast<CCMenuItemSpriteExtra*>(levelcell->m_mainLayer->getChildByIDRecursive("view-button"));
    auto playBtnParent = levelcell->getChildByIDRecursive("main-menu") ?: menu;

    if (playBtnParent && playBtn) {
        auto newPlayBtn = Build<CCSprite>::createSpriteName("GJ_playBtn2_001.png")
            .scale(0.75f)
            .intoMenuItem([this, levelcell](auto) {
                auto layer = LevelInfoLayer::create(m_level, false);
                globed::pushScene(layer);
            })
            .pos(playBtn->getPosition())
            .parent(playBtnParent);

        playBtn->setVisible(false);
    }

    auto diffContainer = levelcell->m_mainLayer->getChildByIDRecursive("difficulty-container");
    if (diffContainer != nullptr) {
        diffContainer->setPositionX(diffContainer->getPositionX() - 2.f);
    }

    // id
    CCNode* editionNode = Build<CCNode>::create()
        .pos({0, CELL_HEIGHT + 10.f})
        .scale(0.6f)
        .parent(m_loadedContainer);

    CCSprite* editionBadge = Build<CCSprite>::create("icon-edition.png"_spr)
        .pos({16.f, -0.5f})
        .scale(0.45f)
        .parent(editionNode);

    auto editionLabel = Build<GradientLabel>::create(fmt::format("#{}", m_levelMeta->edition), "bigFont.fnt")
        .scale(0.60f)
        .anchorPoint({0, 0.5})
        .pos({10.f + editionBadge->getScaledContentWidth(), 0})
        .parent(editionNode)
        .collect();

    editionLabel->setGradientColors({
        color3FromHex("#ffb566"),
        color3FromHex("#fff38f")
    });
    editionLabel->setGradientSpeed(3.f);

    CCScale9Sprite* editionBG = Build<CCScale9Sprite>::create("square02_small.png")
        .opacity(75)
        .zOrder(-1)
        .anchorPoint({0, 0.5})
        .contentSize({editionBadge->getScaledContentWidth() + editionLabel->getScaledContentWidth() + 16.f, 30.f})
        .parent(editionNode);

    globed::findAndAttachRatingSprite(levelcell, m_levelMeta->rateTier);

    // player count label
    m_playerCountContainer = Build<CCNode>::create()
        .layout(RowLayout::create()->setGap(2.f)->setAutoScale(false))
        .pos({CELL_WIDTH - 2.f, CELL_HEIGHT + 3.f})
        .contentSize(20.f, 0.f)
        .anchorPoint(1.f, 0.f)
        .scale(0.6f)
        .parent(m_loadedContainer)
        .collect();

    m_playerCountIcon = Build<CCSprite>::create("featured-player-icon.png"_spr)
        .scale(0.45f)
        .parent(m_playerCountContainer)
        .collect();

    m_playerCountLabel = Build<GradientLabel>::create("0", "bigFont.fnt")
        .scale(0.60f)
        .parent(m_playerCountContainer);

    m_playerCountLabel->setGradientColors({
        color3FromHex("#ffb566"),
        color3FromHex("#fff38f")
    });
    m_playerCountLabel->setGradientSpeed(3.f);

    this->updatePlayerCount(0);
}

void FeaturedLevelCell::updatePlayerCount(uint16_t count) {
    m_playerCountLabel->setString(fmt::format("{}", count));

    cue::resetNode(m_playersBg);

    m_playerCountContainer->setContentWidth(
        m_playerCountLabel->getScaledContentWidth() +
        2.f +
        m_playerCountIcon->getScaledContentWidth()
    );
    m_playerCountContainer->updateLayout();
    log::debug("Container: {}", m_playerCountContainer);
    m_playersBg = cue::attachBackground(m_playerCountContainer, cue::BackgroundOptions{
        .sidePadding = 6.f,
    });
    log::debug("Bg: {}", m_playersBg);
}

static Ref<GJGameLevel> g_cachedLevel;

void FeaturedLevelCell::reload(bool fromFullReload) {
    if (!fromFullReload) this->showLoading();
    this->removeLoadedElements();

    auto flevel = NetworkManagerImpl::get().getFeaturedLevel();
    if (!flevel) {
        cue::resetNode(m_bg);
        cue::resetNode(m_loadingCircle);
        globed::alert("Error", "No featured level is currently available.");
        return;
    }

    m_levelMeta = std::move(flevel);

    // see if we have the level cached
    if (g_cachedLevel && g_cachedLevel->m_levelID == m_levelMeta->levelId) {
        m_level = g_cachedLevel;
        this->createCell();
        this->hideLoading();
        return;
    }

    auto glm = GameLevelManager::get();
    glm->m_levelManagerDelegate = this;
    glm->getOnlineLevels(GJSearchObject::create(SearchType::Search, fmt::to_string(m_levelMeta->levelId)));
}

void FeaturedLevelCell::clearCachedLevel() {
    g_cachedLevel = nullptr;
}

void FeaturedLevelCell::reloadFull() {
    clearCachedLevel();
    this->removeLoadedElements();
    this->showLoading();
    NetworkManagerImpl::get().sendGetFeaturedLevel();
}

void FeaturedLevelCell::removeLoadedElements() {
    m_loadedContainer->removeAllChildren();
    m_loadedBg->setVisible(false);
    m_bg->setVisible(true);
}

void FeaturedLevelCell::showLoading() {
    m_loadingCircle->fadeIn();
    m_loadingCircle->setVisible(true);
}

void FeaturedLevelCell::hideLoading() {
    m_loadingCircle->fadeOut();
}

void FeaturedLevelCell::loadLevelsFinished(CCArray* levels, char const* key, int p2) {
    if (levels && levels->count() > 0) {
        this->levelLoaded(Ok(static_cast<GJGameLevel*>(levels->objectAtIndex(0))));
    } else {
        log::warn("FeaturedLevelCell::loadLevelsFinished called with no levels");
        this->levelLoaded(Err(-1));
    }
}

void FeaturedLevelCell::loadLevelsFailed(char const* key, int p1) {
    this->levelLoaded(Err(p1));
}

void FeaturedLevelCell::levelLoaded(Result<GJGameLevel*, int> result) {
    GameLevelManager::get()->m_levelManagerDelegate = nullptr;
    this->hideLoading();

    if (result.isErr()) {
        globed::alertFormat("Error", "Failed to load featured level (code {})", result.unwrapErr());
        return;
    }

    m_level = result.unwrap();
    g_cachedLevel = m_level;
    this->createCell();
}

FeaturedLevelCell* FeaturedLevelCell::create() {
    auto ret = new FeaturedLevelCell();
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

}
