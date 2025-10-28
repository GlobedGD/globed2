#include "EmoteListPopup.hpp"
#include <globed/core/PopupManager.hpp>
#include <core/hooks/GJBaseGameLayer.hpp>

#include <UIBuilder.hpp>
#include <asp/time/Instant.hpp>

using namespace geode::prelude;
using namespace asp::time;

namespace globed {

const CCSize EmoteListPopup::POPUP_SIZE {340.f, 280.f};
static constexpr CCSize LIST_SIZE = {260.f, 130.f};
static constexpr float CELL_HEIGHT = 40.f;
static constexpr CCSize CELL_SIZE{LIST_SIZE.width, CELL_HEIGHT};
const int EMOTES_PER_PAGE = 18;

static std::optional<Instant> g_lastEmoteTime;

bool EmoteListPopup::setup() {
    this->setTitle("Emotes");
    m_title->setPosition(this->fromTop(15.f));

    m_noElasticity = true;

    m_validEmoteIds = this->getValidEmoteIds(1000);
    m_maxPages = (m_validEmoteIds.size() + EMOTES_PER_PAGE - 1) / EMOTES_PER_PAGE;

    for (int i = 0; i < 4; i++) {
        auto emote = Mod::get()->getSavedValue<uint32_t>(fmt::format("emote-slot-{}", i), 0);
        m_favoriteEmoteIds.push_back(emote);
    }

    m_pageLabel = Build<CCLabelBMFont>::create(fmt::format("Page {} of {}", m_selectedPage + 1, m_maxPages).c_str(), "bigFont.fnt")
        .anchorPoint(0.5f, 0.5f)
        .pos(this->fromTop(30.f))
        .scale(0.3f)
        .zOrder(30)
        .parent(m_mainLayer);

    m_list = Build(cue::ListNode::create(LIST_SIZE, cue::Brown, cue::ListBorderStyle::Comments))
        .anchorPoint(0.5f, 1.f)
        .pos(this->fromTop(40.f))
        .parent(m_mainLayer);

    m_list->updateLayout();

    m_emoteMenu = Build<CCMenu>::create()
        .pos(this->fromTop(45.f))
        .contentSize({245.f, LIST_SIZE.height - 10.f})
        .anchorPoint(0.5f, 1.f)
        .layout(AxisLayout::create()
            ->setAxis(Axis::Row)
            ->setAxisAlignment(AxisAlignment::Start)
            ->setAutoScale(false)
            ->setGrowCrossAxis(true)
            ->setGap(7.f)
        )
        .parent(m_mainLayer);

    this->loadEmoteListPage(0);

    auto bottomMenu = Build<CCMenu>::create()
        .pos(this->fromBottom(25.f))
        .contentSize({0, 0})
        .scale(0.66f)
        .parent(m_mainLayer);
    m_submitBtnSpr = ButtonSprite::create("Send Emote", "bigFont.fnt", "GJ_button_01.png");
    m_submitBtnSpr->setColor({100, 100, 100});
    m_submitBtnSpr->setCascadeColorEnabled(true);
    m_submitBtn = Build(m_submitBtnSpr)
        .intoMenuItem([this] {
            this->onSubmitBtn();
        })
        .id("emote-submit-btn"_spr)
        .scaleMult(1.15f)
        .parent(bottomMenu);
    m_submitBtn->setEnabled(false);

    auto infoMenu = Build<CCMenu>::create()
        .scale(0.75f)
        .pos(this->fromTopRight(16.f, 16.f))
        .contentSize({0, 0})
        .parent(m_mainLayer);
    auto infoBtn = Build<CCSprite>::createSpriteName("GJ_infoIcon_001.png")
        .intoMenuItem(+[] {
            globed::alert(
                "Emotes Guide",

                "Show an <co>emote</c> above your icon for other players to see in-game. Some emotes have <cp>sound effects</c> when used.\n\n"
                "PC users can set up to four <cy>favorite emotes</c> and quickly use them with <cg>hotkeys</c>.\n\n"
                "There is a <cj>small cooldown</c> between sending emotes to prevent <cr>spam</c>."
            );
        })
        .parent(infoMenu);

    auto pageBtnMenu = Build<CCMenu>::create()
        .pos({0, 0})
        .contentSize(m_mainLayer->getContentSize())
        .parent(m_mainLayer);

    auto leftSpr = CCSprite::createWithSpriteFrameName("GJ_chatBtn_01_001.png");
    m_leftPageBtn = Build(leftSpr)
        .intoMenuItem([this] {
            this->updatePage(false);
        })
        .rotation(90.f)
        .pos(22.f, m_list->getPositionY() - LIST_SIZE.height / 2.f)
        .parent(pageBtnMenu);

    auto rightSpr = CCSprite::createWithSpriteFrameName("GJ_chatBtn_01_001.png");
    rightSpr->setFlipY(true);
    m_rightPageBtn = Build(rightSpr)
        .intoMenuItem([this] {
            this->updatePage(true);
        })
        .rotation(90.f)
        .pos(m_mainLayer->getContentWidth() - 22.f, m_list->getPositionY() - LIST_SIZE.height / 2.f)
        .parent(pageBtnMenu);




    m_bottomList = Build(cue::ListNode::create({LIST_SIZE.width, 35.f}, cue::Brown, cue::ListBorderStyle::Comments))
        .anchorPoint(0.5f, 0.5f)
        .pos(this->fromBottom(68.f))
        .parent(m_mainLayer);

    auto favoriteLabel = Build<CCLabelBMFont>::create("Favorites", "goldFont.fnt")
        .scale(0.4f)
        .pos(this->fromBottom(95.f))
        .anchorPoint(0.5f, 0.5f)
        .parent(m_mainLayer);

    m_favoriteEmotesMenu = Build<CCMenu>::create()
        .pos(this->fromBottom(70.f))
        .contentSize({LIST_SIZE.width + 25.f, 0})
        .anchorPoint(0.5f, 0.5f)
        .scale(0.7f)
        .layout(AxisLayout::create()
            ->setAxis(Axis::Row)
            ->setAxisAlignment(AxisAlignment::Center)
            ->setAutoScale(false)
            ->setGap(50.f)
        )
        .parent(m_mainLayer);

    this->loadFavoriteEmotesList();

    m_favoriteHighlight = Build<CCScale9Sprite>::create("emote-btn-back.png"_spr)
        .contentSize({LIST_SIZE.width + 5.f, LIST_SIZE.height + 5.f})
        .pos(m_list->getPosition() + ccp(0, 2.5f))
        .anchorPoint(0.5f, 1.f)
        .color(ccColor3B(255, 230, 0))
        .opacity(30)
        .visible(false)
        .parent(m_mainLayer);
    m_favoriteHighlight->runAction(
        CCRepeatForever::create(
            CCSequence::create(
                CCFadeTo::create(0.8f, 150),
                CCFadeTo::create(0.8f, 30),
                nullptr
            )
        )
    );

    m_favoriteInfoLabel = Build<CCLabelBMFont>::create(fmt::format("Choose an emote to add to favorites (Slot X)").c_str(), "bigFont.fnt")
        .scale(0.3f)
        .color(255, 230, 0)
        .pos(m_list->getPosition() + ccp(0, -LIST_SIZE.height - 2.f))
        .anchorPoint(0.5f, 0.5f)
        .zOrder(20)
        .visible(false)
        .parent(m_mainLayer);

    auto clearFavMenu = Build<CCMenu>::create()
        .pos({m_mainLayer->getContentWidth() - 40.f, 20.f})
        .contentSize({0, 0})
        .scale(0.4f)
        .parent(m_mainLayer);
    m_clearFavoriteBtn = Build(ButtonSprite::create("Remove", "goldFont.fnt", "GJ_button_05.png"))
        .intoMenuItem([this] {
            this->setFavorite(m_selectingFavoriteSlot, 0);
        })
        .id("emote-clear-btn"_spr)
        .scaleMult(1.15f)
        .parent(clearFavMenu);
    m_clearFavoriteBtn->setEnabled(false);
    m_clearFavoriteBtn->setVisible(false);

    return true;
}


void EmoteListPopup::onSubmitBtn() {
    if (m_selectedEmoteId == (uint32_t)-1) {
        // do nothing (this shouldn't happen)
        return;
    }

    // check cooldown
    if (!g_lastEmoteTime || g_lastEmoteTime->elapsed().seconds<float>() > 2.5f) {
        g_lastEmoteTime = Instant::now();
    } else {
        // idk how to do this better
        m_submitBtnSpr->runAction(CCSequence::create(
            CCTintTo::create(0.05f, 255, 0, 0),
            CCTintTo::create(0.3f, 255, 255, 255),
            nullptr
        ));

        return;
    }

    auto gjbgl = globed::GlobedGJBGL::get();
    gjbgl->playSelfEmote(m_selectedEmoteId);

    this->onClose(this);

    if (auto pause = CCScene::get()->getChildByType<PauseLayer>(0)) {
        pause->onResume(pause);
    }
}

void EmoteListPopup::loadEmoteListPage(int page) {
    m_emoteMenu->removeAllChildrenWithCleanup(true);

    int startIdx = page * EMOTES_PER_PAGE;
    int endIdx = std::min(startIdx + EMOTES_PER_PAGE, static_cast<int>(m_validEmoteIds.size()));

    for (int i = startIdx; i < endIdx; i++) {
        uint32_t emoteId = m_validEmoteIds.at(i);

        bool selected = (emoteId == m_selectedEmoteId);

        CCMenuItemSpriteExtra* emoteBtn = Build<CCScale9Sprite>::create("emote-btn-back.png"_spr)
            .contentSize(35.f, 35.f)
            .color(selected ? ccColor3B(0, 255, 0) : ccColor3B(0, 0, 0))
            .opacity(180)
            .intoMenuItem([this, emoteId] {
                this->onEmoteBtn(emoteId);
            })
            .id(fmt::format("emote-btn-{}"_spr, emoteId))
            .scaleMult(1.1f)
            .parent(m_emoteMenu);

        CCSprite* emoteSpr = Build<CCSprite>::createSpriteName(fmt::format("emote_{}.png"_spr, emoteId).c_str())
                .pos(emoteBtn->getContentSize() / 2.f);
        emoteBtn->addChild(emoteSpr, 5);
        cue::rescaleToMatch(emoteSpr, 25.f);

        for (int i = 0; i < m_favoriteEmoteIds.size(); i++) {
            if (m_favoriteEmoteIds.at(i) == emoteId) {
                CCSprite* star = Build<CCSprite>::createSpriteName("GJ_starsIcon_001.png")
                    .scale(0.35f)
                    .zOrder(5)
                    .pos(emoteBtn->getContentWidth() - 6.f, 6.f);
                emoteBtn->addChild(star);
                break;
            }
        }
    }

    m_emoteMenu->updateLayout();
}

std::vector<uint32_t> EmoteListPopup::getValidEmoteIds(uint32_t maxTries) {
    std::vector<uint32_t> validEmoteIds;

    for (uint32_t i = 1; i <= maxTries; i++) {
        auto testEmote = CCSprite::createWithSpriteFrameName(fmt::format("emote_{}.png"_spr, i).c_str());
        if (testEmote) validEmoteIds.push_back(i);
    }

    return validEmoteIds;
}

void EmoteListPopup::onEmoteBtn(uint32_t id) {
    // Emote select mode
    if (!m_isFavoriteMode) {
        m_selectedEmoteId = id;
        m_submitBtn->setEnabled(true);
        m_submitBtnSpr->setColor({255, 255, 255});

        this->loadEmoteListPage(m_selectedPage);
    } else { // Favorite emote select mode
        setFavorite(m_selectingFavoriteSlot, id);
    }
}

void EmoteListPopup::setFavorite(int emoteSlot, uint32_t id) {
    m_isFavoriteMode = false;
    m_favoriteEmoteIds[m_selectingFavoriteSlot] = id;
    m_selectingFavoriteSlot = -1;
    this->loadFavoriteEmotesList();
    this->loadEmoteListPage(m_selectedPage);
    m_favoriteHighlight->setVisible(false);
    m_favoriteInfoLabel->setVisible(false);

    m_clearFavoriteBtn->setEnabled(false);
    m_clearFavoriteBtn->setVisible(false);

    Mod::get()->setSavedValue(fmt::format("emote-slot-{}", emoteSlot), id);
}

void EmoteListPopup::updatePage(bool increment) {
    if (increment) {
        if (m_selectedPage < m_maxPages - 1) {
            m_selectedPage++;
        } else {
            m_selectedPage = 0;
        }
    } else {
        if (m_selectedPage > 0) {
            m_selectedPage--;
        } else {
            m_selectedPage = m_maxPages - 1;
        }
    }
    m_pageLabel->setString(fmt::format("Page {} of {}", m_selectedPage + 1, m_maxPages).c_str());
    this->loadEmoteListPage(m_selectedPage);
}

void EmoteListPopup::enterFavoriteSelectMode(int emoteSlot) {
    m_isFavoriteMode = true;
    m_selectingFavoriteSlot = emoteSlot;
    this->loadFavoriteEmotesList();
    m_favoriteHighlight->setVisible(true);
    m_favoriteInfoLabel->setString(fmt::format("Choose an emote to add to favorites (Slot {})", emoteSlot + 1).c_str());
    m_favoriteInfoLabel->setVisible(true);

    m_clearFavoriteBtn->setEnabled(true);
    m_clearFavoriteBtn->setVisible(true);
}

void EmoteListPopup::loadFavoriteEmotesList() {
    m_favoriteEmotesMenu->removeAllChildrenWithCleanup(true);

    for (int i = 0; i < m_favoriteEmoteIds.size(); i++) {
        uint32_t emoteId = m_favoriteEmoteIds.at(i);
        bool selected = (i == m_selectingFavoriteSlot);

        CCMenuItemSpriteExtra* emoteBtn = Build<CCScale9Sprite>::create("emote-btn-back.png"_spr)
            .contentSize(35.f, 35.f)
            .color(selected ? ccColor3B{255, 230, 0} : ccColor3B{0, 0, 0})
            .opacity(180)
            .intoMenuItem([this, i] {
                this->enterFavoriteSelectMode(i);
            })
            .id(fmt::format("emote-favorite-btn-{}"_spr, i))
            .scaleMult(1.1f)
            .parent(m_favoriteEmotesMenu);

        CCSprite* emoteSpr = Build<CCSprite>::createSpriteName(fmt::format("emote_{}.png"_spr, emoteId).c_str())
                .pos(emoteBtn->getContentSize() / 2.f);
        emoteBtn->addChild(emoteSpr, 1);
        cue::rescaleToMatch(emoteSpr, 25.f);

        CCLabelBMFont* slotLabel = Build<CCLabelBMFont>::create(fmt::format("{}", i + 1).c_str(), "goldFont.fnt")
            .scale(0.66f)
            .pos(emoteBtn->getContentSize().width / 2.f, -1.f);
        emoteBtn->addChild(slotLabel);
    }

    m_favoriteEmotesMenu->updateLayout();
}

}