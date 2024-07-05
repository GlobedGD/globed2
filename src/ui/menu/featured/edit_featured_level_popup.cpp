#include "edit_featured_level_popup.hpp"

#include <Geode/ui/TextInput.hpp>

#include <managers/error_queues.hpp>
#include <managers/daily_manager.hpp>
#include <util/ui.hpp>
#include <util/format.hpp>

using namespace geode::prelude;

bool EditFeaturedLevelPopup::setup() {
    this->setTitle("Globed: Suggest Level", "bigFont.fnt", 0.75f, 20.f);

    auto rlayout = util::ui::getPopupLayoutAnchored(m_size);

    // Build<TextInput>::create(POPUP_WIDTH * 0.8f, "Level ID", "chatFont.fnt")
    //     .pos(rlayout.fromTop(50.f))
    //     .parent(m_mainLayer)
    //     .store(idInput);

    // idInput->setCommonFilter(CommonFilter::Int);

    Build<ButtonSprite>::create("Set", "bigFont.fnt", "GJ_button_01.png", 0.9f)
        .scale(0.75f)
        .intoMenuItem([this] {
            this->save();
        })
        .pos(rlayout.fromBottom(22.f))
        .intoNewParent(CCMenu::create())
        .pos(0.f, 0.f)
        .parent(m_mainLayer);

    Build<CCMenu>::create()
        .pos(rlayout.center)
        .parent(m_mainLayer)
        .store(menu);

    int diff = 0;
    // would a backwards wormhole be a whitehole or a holeworm?
    if (level->m_autoLevel) 
        diff = -1;
    else if (level->m_ratingsSum != 0) {
        if (level->m_demon == 1){
            int fixedNum = level->m_demonDifficulty;

            if (fixedNum != 0)
                fixedNum -= 2;

            diff = 6 + fixedNum;
        }
        else{
            diff = level->m_ratingsSum / level->m_ratings;
        }
    }
    else
        diff = 0;

    log::info("diff {}", diff);
    Build<GJDifficultySprite>::create(diff, GJDifficultyName::Short)
        .pos(rlayout.center)
        .parent(m_mainLayer)
        .scale(1.25)
        .zOrder(3);

    // toggler
    this->createDiffButton();

    return true;
}

void EditFeaturedLevelPopup::save() {
    // int levelId = util::format::parse<int>(idInput->getString()).value_or(0);
    int levelId = this->level->m_levelID;

    if (levelId == 0) {
        ErrorQueues::get().warn("Invalid level ID");
        return;
    }

    auto req = WebRequestManager::get().setFeaturedLevel(levelId, currIdx);
    reqListener.bind(this, &EditFeaturedLevelPopup::onRequestComplete);
    reqListener.setFilter(std::move(req));
}

void EditFeaturedLevelPopup::onRequestComplete(typename WebRequestManager::Event* event) {
    if (!event || !event->getValue()) return;

    auto result = std::move(*event->getValue());

    if (result.isOk()) {
        ErrorQueues::get().success("Successfully updated the level");
    } else {
        auto err = result.unwrapErr();
        ErrorQueues::get().error(fmt::format("Failed to update the level.\n\nReason: <cy>{}</c>", util::format::webError(err)));
    }
}

void EditFeaturedLevelPopup::createDiffButton() {
    if (curDiffButton) curDiffButton->removeFromParent();

    // const char* name;
    // switch (currIdx) {
    //     case 2: name = "Outstanding"; break;
    //     case 1: name = "Epic"; break;
    //     case 0:
    //     default:
    //         name = "Featured"; break;
    // }

    auto rlayout = util::ui::getPopupLayoutAnchored(m_size);
    
    // auto makeSprite = [this]{
    //     return ButtonSprite::createWithSpriteFrameName(
    //         "menuicon.png"_spr,
    //         1.f,
    //         m_fields->btnActive ? CircleBaseColor::Cyan : CircleBaseColor::Green,
    //         CircleBaseSize::MediumAlt
    //     );
    // };

    // Build<CCMenuItemSpriteExtra>::create(DailyManager::get().createRatingSprite())
    //     .scale(0.8f)
    //     .intoMenuItem([this] {
    //         this->currIdx++;
    //         if (this->currIdx > 2) {
    //             this->currIdx = 0;
    //         }

    //         this->createDiffButton();
    //     })
    //     .pos(rlayout.fromBottom(55.f))
    //     .intoNewParent(CCMenu::create())
    //     .store(curDiffButton)
    //     .pos(0.f, 0.f)
    //     .parent(m_mainLayer);
    // ;

    auto spr = DailyManager::get().createRatingSprite(currIdx);
    spr->setScale(1.25);

    Build<CCMenuItemSpriteExtra>::create(spr, this, menu_selector(EditFeaturedLevelPopup::onDiffClick))
        .store(curDiffButton)
        .parent(menu);
}

void EditFeaturedLevelPopup::onDiffClick(CCObject* sender) {
    this->currIdx++;
    if (this->currIdx > 2) {
        this->currIdx = 0;
    }

    this->createDiffButton();
}

EditFeaturedLevelPopup* EditFeaturedLevelPopup::create(GJGameLevel* level) {
    auto ret = new EditFeaturedLevelPopup();
    ret->level = level; // lol
    if (ret->initAnchored(POPUP_WIDTH, POPUP_HEIGHT)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}
